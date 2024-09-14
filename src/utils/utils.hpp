#ifndef UTILS_H_
#define UTILS_H_

#include <atomic>
#include <barrier>
#include <functional>
#include <mutex>
#include <queue>
#include <semaphore>
#include <shared_mutex>
#include <type_traits>

/*
template <typename T> class safe_vector {
    T* _data = nullptr;
    size_t _capacity = 0;
    std::atomic<size_t> _size = 0;
    std::mutex _reserve_lock;

  public:
    // returns index of item it pushed back
    size_t push_back(T const& item) {
        size_t idx = _size.fetch_add(1, std::memory_order_relaxed);
        ensure_capacity(idx);
        _data[idx] = item;
        return idx;
    }
    void reserve(size_t capacity) {
        if (capacity > _capacity) {
            std::unique_lock<std::mutex> lock(_reserve_lock);
            T* tmp = reinterpret_cast<T*>(new char[capacity * sizeof(T)]);
            if (_data != nullptr) {
                memcpy(tmp, _data, _capacity * sizeof(T));
            }
            _data = tmp;
            _capacity = capacity;
        }
    }
    void grow(size_t grow_size) { reserve(grow_size + size()); }
    inline T& operator[](size_t const& idx) { return _data[idx]; }
    void clear() { _size.store(0, std::memory_order_seq_cst); }
    size_t size() const { return _size.load(std::memory_order_seq_cst); }

  private:
    void ensure_capacity(size_t const& idx) {
        if (idx > _capacity || _capacity == 0) {
            throw std::runtime_error("not enough capacity in safe vector for index: " + std::to_string(idx));
        }
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

template <typename T> class safe_deque {};
*/

template <typename T> class safe_vector {
    std::vector<T> _vec;
    std::shared_mutex _mutex;

  public:
    void push_back(const T& item) {
        std::unique_lock<std::shared_mutex> _mutex;
        _vec.push_back(item);
    }
    void push_back(T& item) {
        std::unique_lock<std::shared_mutex> _mutex;
        _vec.push_back(item);
    }
    void reserve(size_t const& size) {
        std::unique_lock<std::shared_mutex> _mutex;
        _vec.reserve(size);
    }
    T at(size_t const& index) {
        std::shared_lock<std::shared_mutex> _mutex;
        return _vec[index];
    }
    void clear() {
        std::unique_lock<std::shared_mutex> _mutex;
        _vec.clear();
    }
    void erase(size_t const& index) {
        std::unique_lock<std::shared_mutex> _mutex;
        _vec.erase(_vec.begin() + index);
        //_vec[index] = _vec.back();
    }
    // NOTE:
    // This isn't thread safe
    inline T& operator[](size_t const& index) { return _vec[index]; }
    size_t size() {
        std::unique_lock<std::shared_mutex> _mutex;
        return _vec.size();
    }
    void grow(size_t grow_size) {
        std::unique_lock<std::shared_mutex> _mutex;
        _vec.reserve(grow_size + _vec.size());
    }
};

template <typename T> class safe_queue {
    std::queue<T> _queue;
    std::mutex _mutex;

  public:
    void push(const T& item) {
        std::unique_lock<std::mutex> _mutex;
        _queue.push(item);
    }
    void push(T& item) {
        std::unique_lock<std::mutex> _mutex;
        _queue.push(item);
    }
    // NOTE:
    // need pop and front to be the same because of multithreading,
    // so they just get combined into pop
    T pop() {
        std::unique_lock<std::mutex> _mutex;
        T item = _queue.front();
        _queue.pop();
        return item;
    }
    bool empty() {
        std::unique_lock<std::mutex> _mutex;
        return _queue.empty();
    }
};

template <typename T> class safe_deque {
    std::deque<T> _deque;
    std::mutex _mutex;

  public:
    void push_back(const T& item) {
        std::unique_lock<std::mutex> _mutex;
        _deque.push_back(item);
    }
    void push_front(T& item) {
        std::unique_lock<std::mutex> _mutex;
        _deque.push_front(item);
    }
    T pop_back() {
        std::unique_lock<std::mutex> _mutex;
        T item = _deque.back();
        _deque.pop_back();
        return item;
    }
    T pop_front() {
        std::unique_lock<std::mutex> _mutex;
        T item = _deque.front();
        _deque.pop_front();
        return item;
    }
    bool empty() {
        std::unique_lock<std::mutex> _mutex;
        return _deque.empty();
    }
    size_t size() {
        std::unique_lock<std::mutex> _mutex;
        return _deque.size();
    }
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
