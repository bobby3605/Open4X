#ifndef UTILS_H_
#define UTILS_H_

#include <barrier>
#include <condition_variable>
#include <cstring>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <type_traits>

// thread safe queue
template <typename T> class safe_queue {
  public:
    ~safe_queue() { exit(); }
    void push(T item) {
        std::unique_lock<std::mutex> lock(_mutex);
        _queue.push(item);
        _cond.notify_one();
    }
    T pop() {
        std::unique_lock<std::mutex> lock(_mutex);
        _cond.wait(lock, [&]() { return _should_exit || !_queue.empty(); });
        if (_should_exit) {
            return nullptr;
        }
        T item = _queue.front();
        _queue.pop();
        return item;
    }
    bool empty() {
        std::unique_lock<std::mutex> lock(_mutex);
        return _queue.empty();
    }

    void exit() {
        _should_exit = true;
        _cond.notify_all();
    }

  private:
    std::queue<T> _queue;
    std::mutex _mutex;
    std::condition_variable _cond;
    std::atomic<bool> _should_exit = false;
};

template <typename T> class safe_queue_external_sync {
  public:
    void push(T item) {
        std::unique_lock<std::mutex> lock(_mutex);
        _queue.push(item);
    }
    T pop() {
        std::unique_lock<std::mutex> lock(_mutex);
        if (_queue.empty()) {
            return nullptr;
        }
        T item = _queue.front();
        _queue.pop();
        return item;
    }
    bool empty() {
        std::unique_lock<std::mutex> lock(_mutex);
        return _queue.empty();
    }

  private:
    std::queue<T> _queue;
    std::mutex _mutex;
};

template <typename T> class safe_deque {
  public:
    safe_deque() {
        // NOTE:
        // This must be here in order for the indices to work properly
        grow(2);
    }
    ~safe_deque() {
        if (_data != nullptr) {
            free(_data);
        }
    }
    void push_front(T const& item) {
        // - 1 pre increment because _front_idx starts at 0
        size_t idx = --_front_idx;
        safe_write(idx, item);
    }
    void push_back(T const& item) {
        size_t idx = _back_idx++;
        safe_write(idx, item);
    }
    void pop_front() { ++_front_idx; }
    void pop_back() { --_back_idx; }
    T front() { return _data[_base_index + _front_idx]; }
    T back() { return _data[_base_index + _back_idx]; }
    void reserve(size_t count) { grow(count); }
    bool empty() { return size() == 0; }
    int size() {
        // NOTE:
        // - _front_idx because _front_idx is negative,
        // so this will return the total amount of alloced Ts
        return _back_idx - _front_idx;
    }

  private:
    void safe_write(int idx, T const& item) {
        size_t tmp_base_index = _base_index;
        ensure_space(tmp_base_index, idx);
        _data[tmp_base_index + idx] = item;
    }
    void ensure_space(int tmp_base_index, int idx) {
        // base_index points to the middle of the _data block, and is how many indices have been allocated on one side,
        // if the abs(idx) to write to is greater than the number of indices allocated on one side
        // grow the data to ensure space
        if (abs(idx) > tmp_base_index || _data == nullptr) {
            // NOTE:
            // there's probably a race condition here where extra blocks could be allocated,
            // but to fix it this function would need a mutex
            // the extra performance is probably worth some extra memory usage
            // grow(1);
            // NOTE:
            // ensuring that push cannot cause a reallocation makes front() and back() trivially thread safe,
            // because neither _data nor _base_index will change, and _x_idx is an atomic
            throw std::runtime_error("safe_deque out of space");
        }
    }
    void grow(size_t count) {
        std::unique_lock<std::mutex> lock(_grow_lock);
        // put one extra block on each side
        size_t grow_count = 2 * count;
        size_t current_count = _base_index * 2;
        size_t new_count = current_count + grow_count;
        size_t new_byte_size = new_count * sizeof(T);
        T* tmp = reinterpret_cast<T*>(new char[new_byte_size]);
        _base_index = new_count / 2;
        if (_data != nullptr) {
            // copy data to tmp so that there's grow_size extra blocks on the left and right
            // NOTE:
            // grow_count / 2 = count, so just using count here
            std::memcpy(tmp + count, _data, current_count * sizeof(T));
            free(_data);
        }
        _data = tmp;
    }
    std::atomic<int> _front_idx = 0;
    std::atomic<int> _back_idx = 0;
    T* _data = nullptr;
    std::atomic<int> _base_index = 0;
    std::mutex _grow_lock;
    std::atomic<int> _size = 0;
};

template <typename T> class atomic_deque {
    std::atomic<int> _front_idx = 0;
    std::atomic<int> _back_idx = 0;
    T* _data = nullptr;
    std::atomic<int> _base_index = 0;

  public:
    atomic_deque() {
        // NOTE:
        // This must be here in order for the indices to work properly
        grow(2);
    }
    ~atomic_deque() { free(_data); }
    void push_front(T const& item) {
        // - 1 pre increment because _front_idx starts at 0
        size_t idx = --_front_idx;
        _data[_base_index + idx] = item;
    }
    void push_back(T const& item) {
        size_t idx = _back_idx++;
        _data[_base_index + idx] = item;
    }
    void pop_front() { ++_front_idx; }
    void pop_back() { --_back_idx; }
    T front() { return _data[_base_index + _front_idx]; }
    T back() { return _data[_base_index + _back_idx]; }
    void reserve(size_t count) { grow(count); }
    bool empty() { return size() == 0; }
    int size() {
        // NOTE:
        // - _front_idx because _front_idx is negative,
        // so this will return the total amount of alloced Ts
        return _back_idx - _front_idx;
    }

  protected:
    // NOTE:
    // This is not thread safe
    void grow(size_t count) {
        // put one extra block on each side
        size_t grow_count = 2 * count;
        size_t current_count = _base_index * 2;
        size_t new_count = current_count + grow_count;
        size_t new_byte_size = new_count * sizeof(T);
        T* tmp = reinterpret_cast<T*>(new char[new_byte_size]);
        _base_index = new_count / 2;
        if (_data != nullptr) {
            // copy data to tmp so that there's grow_size extra blocks on the left and right
            // NOTE:
            // grow_count / 2 = count, so just using count here
            std::memcpy(tmp + count, _data, current_count * sizeof(T));
            free(_data);
        }
        _data = tmp;
    }
    template <typename> friend class safe_deque_v2;
};

template <typename T> class safe_deque_v2 {
    atomic_deque<T> _atomic_deque;
    std::atomic<size_t> _realloc_size = 0;
    std::atomic<size_t> _in_progress_threads = 0;

  public:
    void push_front(T const& item) {
        atomic_wait_eq(_realloc_size, 0);
        realloc_check(0);
        ++_in_progress_threads;
        _atomic_deque.push_front(item);
        --_in_progress_threads;
    }
    void push_back(T const& item) {
        atomic_wait_eq(_realloc_size, 0);
        realloc_check(0);
        ++_in_progress_threads;
        _atomic_deque.push_back(item);
        --_in_progress_threads;
    }
    void pop_front() {
        atomic_wait_eq(_realloc_size, 0);
        ++_in_progress_threads;
        _atomic_deque.pop_front();
        --_in_progress_threads;
    }
    void pop_back() {
        atomic_wait_eq(_realloc_size, 0);
        ++_in_progress_threads;
        _atomic_deque.pop_back();
        --_in_progress_threads;
    }
    T front() {
        atomic_wait_eq(_realloc_size, 0);
        ++_in_progress_threads;
        // TODO:
        // there might be an extra copy here that can be optimizied,
        // maybe gcc can handle it anyway
        const T t = _atomic_deque.front();
        --_in_progress_threads;
        return t;
    }
    T back() {
        atomic_wait_eq(_realloc_size, 0);
        ++_in_progress_threads;
        const T t = _atomic_deque.back();
        --_in_progress_threads;
        return t;
    }
    void grow(size_t count) {
        atomic_wait_eq(_realloc_size, 0);
        ++_in_progress_threads;
        _atomic_deque.grow(count);
        --_in_progress_threads;
    }
    bool empty() {
        atomic_wait_eq(_realloc_size, 0);
        ++_in_progress_threads;
        const bool _empty = _atomic_deque.empty();
        --_in_progress_threads;
        return _empty;
    }
    int size() {
        atomic_wait_eq(_realloc_size, 0);
        ++_in_progress_threads;
        const int _size = _atomic_deque.size();
        --_in_progress_threads;
        return _size;
    }

  private:
    template <typename AT> void atomic_wait_eq(std::atomic<AT> const& atomic, AT const& val) {
        // FIXME:
        // This is really bad
        while (atomic != val) {
        }
    }
    void realloc_check(size_t required_size) {
        size_t grow_needed = required_size - size();
        if (grow_needed > 0) {
            size_t tmp_realloc_size = _realloc_size.fetch_add(grow_needed);
            if (tmp_realloc_size == 0) {
                // grow on the thread that asked for the first realloc
                atomic_wait_eq(_in_progress_threads, 0);
                grow(_realloc_size);
                tmp_realloc_size = 0;
            }
        }
    };
};

template <typename T> class safe_deque_v3 {
    std::atomic<int> _front_idx = 0;
    std::atomic<int> _back_idx = 0;
    std::mutex _idx_lock;
    T* _data = nullptr;
    std::atomic<int> _base_index = 0;

  public:
    safe_deque_v3() {
        // NOTE:
        // This must be here in order for the indices to work properly
        grow(2);
    }
    ~safe_deque_v3() { free(_data); }
    void push_front(T const& item) {
        std::unique_lock<std::mutex> lock(_idx_lock);
        // - 1 pre increment because _front_idx starts at 0
        size_t idx = --_front_idx;
        safe_write(idx, item);
    }
    void push_back(T const& item) {
        std::unique_lock<std::mutex> lock(_idx_lock);
        size_t idx = _back_idx++;
        safe_write(idx, item);
    }
    void pop_front() { ++_front_idx; }
    void pop_back() { --_back_idx; }
    T front() { return _data[_base_index + _front_idx]; }
    T back() { return _data[_base_index + _back_idx]; }
    void reserve(size_t count) {
        size_t tmp_size = size();
        if (count > tmp_size) {
            std::unique_lock<std::mutex> lock(_idx_lock);
            grow(count - tmp_size);
        }
    }
    bool empty() { return size() == 0; }
    int size() {
        std::unique_lock<std::mutex> lock(_idx_lock);
        // NOTE:
        // - _front_idx because _front_idx is negative,
        // so this will return the total amount of alloced Ts
        return _back_idx - _front_idx;
    }

  protected:
    void safe_write(int idx, T const& item) {
        size_t tmp_base_index = _base_index;
        ensure_space(tmp_base_index, idx);
        _data[tmp_base_index + idx] = item;
    }
    void ensure_space(int tmp_base_index, int idx) {
        // base_index points to the middle of the _data block, and is how many indices have been allocated on one side,
        // if the abs(idx) to write to is greater than the number of indices allocated on one side
        // grow the data to ensure space
        if (abs(idx) > tmp_base_index || _data == nullptr) {
            grow(abs(idx) - tmp_base_index);
        }
    }
    // NOTE:
    // This is not thread safe
    void grow(size_t count) {
        // put one extra block on each side
        size_t grow_count = 2 * count;
        size_t current_count = _base_index * 2;
        size_t new_count = current_count + grow_count;
        size_t new_byte_size = new_count * sizeof(T);
        T* tmp = reinterpret_cast<T*>(new char[new_byte_size]);
        _base_index = new_count / 2;
        if (_data != nullptr) {
            // copy data to tmp so that there's grow_size extra blocks on the left and right
            // NOTE:
            // grow_count / 2 = count, so just using count here
            std::memcpy(tmp + count, _data, current_count * sizeof(T));
            free(_data);
        }
        _data = tmp;
    }
};

template <typename T> class safe_queue_v2 {
    std::atomic<int> _idx = 0;
    std::mutex _idx_lock;
    T* _data = nullptr;
    std::atomic<int> _current_size = 0;

  public:
    safe_queue_v2() {
        // NOTE:
        // This must be here in order for the indices to work properly
        grow(2);
    }
    ~safe_queue_v2() { free(_data); }
    void push(T const& item) {
        std::unique_lock<std::mutex> lock(_idx_lock);
        safe_write(_idx++, item);
    }
    void pop() { --_idx; }
    T front() { return _data[_idx - 1]; }
    void reserve(size_t count) {
        size_t tmp_size = size();
        if (count > tmp_size) {
            std::unique_lock<std::mutex> lock(_idx_lock);
            grow(count - tmp_size);
        }
    }
    bool empty() { return size() == 0; }
    int size() {
        std::unique_lock<std::mutex> lock(_idx_lock);
        return _idx;
    }

  protected:
    void safe_write(int idx, T const& item) {
        if (idx >= _current_size) {
            grow(idx + 1 - _current_size);
        }
        _data[idx] = item;
    }
    // NOTE:
    // This is not thread safe
    void grow(size_t count) {
        // put one extra block on each side
        size_t new_count = (_current_size + count) * 2;
        size_t new_byte_size = new_count * sizeof(T);
        T* tmp = reinterpret_cast<T*>(new char[new_byte_size]);
        if (_data != nullptr) {
            std::memcpy(tmp, _data, _current_size * sizeof(T));
            free(_data);
        }
        _data = tmp;
        _current_size = new_count;
    }
};

struct VectorSlice {
    size_t offset;
    size_t size;
};

class ThreadPool {

    std::vector<std::thread> _threads;
    size_t _num_threads;
    std::counting_semaphore<> _thread_semaphore;
    std::barrier<> _thread_barrier;
    std::atomic<bool> _stop_threads = false;

  public:
    ThreadPool(size_t const& num_threads, std::function<void(size_t thread_id)> const& work_task);
    ~ThreadPool();
    size_t const& num_threads() const { return _num_threads; }
    void run();
};

template <typename T> class VectorSlicer {
    size_t _num_threads;
    std::vector<T>& _vector;
    std::vector<VectorSlice> _vector_slices;
    std::function<void(size_t i)> _work_task;
    ThreadPool _thread_pool;

  public:
    VectorSlicer(std::vector<T>& vector, size_t const& num_threads, std::function<void(size_t i)> const& work_task)
        : _num_threads(num_threads), _vector(vector), _work_task(work_task), _thread_pool(num_threads, [&](size_t thread_id) {
              VectorSlice& slice = _vector_slices[thread_id];
              size_t end_offset = slice.offset + slice.size;
              for (size_t i = slice.offset; i < end_offset; ++i) {
                  _work_task(i);
              }
          }) {
        _vector_slices.reserve(num_threads);
    }
    void run(VectorSlice const& section_to_slice) {
        // run on this thread if the thread pool would be too much overhead
        if (section_to_slice.size < _num_threads || _num_threads == 0) {
            for (size_t i = section_to_slice.offset; i < (section_to_slice.offset + section_to_slice.size); ++i) {
                _work_task(i);
            }
        } else {
            size_t slice_size = section_to_slice.size / _num_threads;
            size_t remainder = section_to_slice.size % _num_threads;
            size_t curr_offset = section_to_slice.offset;
            for (size_t i = 0; i < _num_threads; ++i) {
                _vector_slices[i].size = slice_size;
                _vector_slices[i].offset = curr_offset;
                curr_offset += slice_size;
                if (i == _num_threads - 1) {
                    _vector_slices[i].size += remainder;
                }
            }
            _thread_pool.run();
        }
    }
};

// partially apply tail arguments
// returns a std::function<ReturnType(FirstArgumentType)>
// could probably be made more generic
template <typename F, typename... Args> auto partial(F f, Args... tail_args) {
    return [=](auto head_arg) { return f(head_arg, tail_args...); };
}

/*
template <typename T, bool = std::is_pointer<T>::value> struct fast_optional {};

template <typename T> struct fast_optional<T, true> {
    fast_optional() {}
    fast_optional(T& value) : _value(value) {}
    T _value = nullptr;
    inline bool has_value() { return _value == nullptr; };
    inline T& value() { return _value; }
};

// value only needs to be a pointer if T isn't
template <typename T> struct fast_optional<T, false> {
    fast_optional() {}
    fast_optional(T& value) : _value(value) {}
    T* _value = nullptr;
    inline bool has_value() { return _value == nullptr; };
    inline T* value() { return _value; }
};
*/

/*
template <typename T, bool = std::is_pointer<T>::value> struct fast_optional {};

template <typename T> struct fast_optional<T, true> {
    fast_optional() {}
    fast_optional(T& value) : _value(value) {}
    T _value = nullptr;
    inline bool has_value() { return _value == nullptr; };
    inline T& value() { return _value; }
    template <typename... Args> fast_optional(Args&&... args) : _value(args...) {}
};

template <typename T> struct fast_optional<T, false> {
    fast_optional() {}
    fast_optional(T& value) : _value(value) {}
    T _value = NULL;
    inline bool has_value() { return _value == NULL; };
    inline T& value() { return _value; }
    template <typename... Args> fast_optional(Args&&... args) : _value(args...) {}
};
*/

/*
template <typename T> struct fast_optional {
    T* _value = nullptr;
    fast_optional() {}
    fast_optional(T& value) : _value(value) {}
    fast_optional(T&& value) : _value(value) {}
    template <typename... Args> fast_optional(Args&&... args) : _value(new T(args...)) {}
    inline bool has_value() { return _value == nullptr; }
    inline T& value() { return *_value; }
};
*/

template <typename T> struct fast_optional {};

template <> struct fast_optional<size_t> {
    size_t _value = -1;
    inline bool has_value() { return _value != (size_t)(-1); }
    inline size_t& value() { return _value; }
    fast_optional() {}
    fast_optional(size_t& value) : _value(value) {}
    fast_optional(size_t&& value) : _value(value) {}
    template <typename OT> void inline operator=(OT&& other) { _value = other; }
};

template <typename T>
    requires(std::is_pointer<T>::value)
struct fast_optional<T> {
    T _value = nullptr;
    inline bool has_value() { return _value != nullptr; }
    inline T& value() { return _value; }
    fast_optional() {}
    template <typename... Args> fast_optional(Args&&... args) {
        _value = new std::remove_pointer<decltype(fast_optional<T>::_value)>::type(args...);
    }
    template <typename OT> void inline operator=(OT&& other) { _value = other; }
    T inline operator->() { return _value; }
};

#endif // UTILS_H_
