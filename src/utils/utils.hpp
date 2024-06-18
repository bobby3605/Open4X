#ifndef UTILS_H_
#define UTILS_H_

#include <atomic>
#include <barrier>
#include <cstring>
#include <functional>
#include <semaphore>
#include <type_traits>

inline void atomic_wait_eq(std::atomic<size_t> const& atomic_val, size_t eq_val) {
    // FIXME:
    // This is really bad
    while (atomic_val.load(std::memory_order_relaxed) != eq_val) {
    }
}

// TODO:
// Write a test for this
template <typename T> class safe_vector {
    T* _data = nullptr;
    size_t _capacity = 0;
    std::atomic<size_t> _size = 0;
    std::atomic<size_t> _in_progress_threads = 0;
    // std::atomic<size_t> _post_wait_in_progress_threads = 0;
    std::atomic<size_t> _grow_size = 0;

  public:
    // returns index of item it pushed back
    size_t push_back(T const& item) {
        size_t idx = _size.fetch_add(1, std::memory_order_relaxed);
        realloc_check(idx);
        _in_progress_threads.fetch_add(1, std::memory_order_relaxed);
        atomic_wait_eq(_grow_size, 0);
        _data[idx] = item;
        _in_progress_threads.fetch_sub(1, std::memory_order_relaxed);
        return idx;
    }
    inline T operator[](size_t const& idx) {
        _in_progress_threads.fetch_add(1, std::memory_order_relaxed);
        atomic_wait_eq(_grow_size, 0);
        T t = _data[idx];
        _in_progress_threads.fetch_sub(1, std::memory_order_relaxed);
        return t;
    }
    void clear() { _size.store(0, std::memory_order_seq_cst); }
    size_t size() const { return _size.load(std::memory_order_seq_cst); }
    void reserve(size_t const& capacity) { realloc_check(capacity); }
    void grow(size_t const& grow_size) { realloc_check(grow_size + size()); }

  private:
    void realloc_check(size_t const& idx) {
        if (idx > _capacity || _capacity == 0) {
            size_t tmp = _grow_size.fetch_add(idx, std::memory_order_relaxed);
            // Run grow on only a single thread
            if (tmp == 0) {
                // wait for in progress threads to complete or get stuck on realloc
                atomic_wait_eq(_in_progress_threads, 0);
                _grow(_grow_size.load(std::memory_order_relaxed));
                _grow_size.store(0, std::memory_order_relaxed);
            } else {
                atomic_wait_eq(_grow_size, 0);
            }
        }
    }
    void _reserve(size_t capacity) {
        if (capacity > _capacity) {
            T* tmp = reinterpret_cast<T*>(new char[capacity * sizeof(T)]);
            if (_data != nullptr) {
                memcpy(tmp, _data, _capacity * sizeof(T));
            }
            _data = tmp;
            _capacity = capacity;
        }
    }
    void _grow(size_t grow_size) {
        // NOTE:
        // size() is probably bigger than the actual size because push_back calls fetch_add, but it's fine to overallocate
        _reserve(grow_size + size());
    }
    template <typename> friend class safe_queue;
};

template <typename T> class safe_queue {
    safe_vector<T> _vector;

  public:
    void push(T const& item) { _vector.push_back(item); }
    void pop() { _vector._size.fetch_sub(1, std::memory_order_relaxed); }
    T front() { return _vector[_vector._size.load(std::memory_order_relaxed) - 1]; }
    void reserve(size_t capacity) { _vector.reserve(capacity); }
    void grow(size_t grow_size) { _vector.grow(grow_size); }
    bool empty() { return _vector.size() == 0; }
};

template <typename T> class safe_deque {
  public:
    safe_deque() {
        // NOTE:
        // This must be here in order for the indices to work properly
        _grow(2);
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
    void pop_front() { _front_idx.fetch_add(1, std::memory_order_relaxed); }
    void pop_back() { _back_idx.fetch_sub(1, std::memory_order_relaxed); }
    T front() {
        _in_progress_threads.fetch_add(1, std::memory_order_relaxed);
        T t = _data[_base_index + _front_idx];
        _in_progress_threads.fetch_sub(1, std::memory_order_relaxed);
        return t;
    }
    T back() {
        _in_progress_threads.fetch_add(1, std::memory_order_relaxed);
        T t = _data[_base_index + _back_idx];
        _in_progress_threads.fetch_sub(1, std::memory_order_relaxed);
        return t;
    }
    void reserve(size_t count) { _grow(count); }
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
        _in_progress_threads.fetch_add(1, std::memory_order_relaxed);
        _data[tmp_base_index + idx] = item;
        _in_progress_threads.fetch_sub(1, std::memory_order_relaxed);
    }
    void ensure_space(int tmp_base_index, int idx) {
        // base_index points to the middle of the _data block, and is how many indices have been allocated on one side,
        // if the abs(idx) to write to is greater than the number of indices allocated on one side
        // grow the data to ensure space
        if (abs(idx) > tmp_base_index || _data == nullptr) {
            size_t tmp = _grow_size.fetch_add(abs(idx), std::memory_order_relaxed);
            // Run grow on only a single thread
            if (tmp == 0) {
                // wait for in progress threads to complete or get stuck on realloc
                atomic_wait_eq(_in_progress_threads, 0);
                _grow(_grow_size.load(std::memory_order_relaxed));
                _grow_size.store(0, std::memory_order_relaxed);
            } else {
                atomic_wait_eq(_grow_size, 0);
            }
        }
    }
    void _grow(size_t count) {
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
    std::atomic<int> _size = 0;
    std::atomic<size_t> _in_progress_threads = 0;
    std::atomic<size_t> _grow_size = 0;
};

struct Chunk {
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

template <typename T, template <class CT> class CT = std::vector> class ChunkProcessor {
    size_t _num_threads;
    CT<T>& _vector;
    std::vector<Chunk> _vector_slices;
    std::function<void(size_t i)> _work_task;
    ThreadPool _thread_pool;

  public:
    ChunkProcessor(CT<T>& vector, size_t const& num_threads, std::function<void(size_t i)> const& work_task)
        : _num_threads(num_threads), _vector(vector), _work_task(work_task), _thread_pool(num_threads, [&](size_t thread_id) {
              Chunk& slice = _vector_slices[thread_id];
              size_t end_offset = slice.offset + slice.size;
              for (size_t i = slice.offset; i < end_offset; ++i) {
                  _work_task(i);
              }
          }) {
        _vector_slices.reserve(num_threads);
    }
    void run(Chunk const& section_to_slice) {
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
