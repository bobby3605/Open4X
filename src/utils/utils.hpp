#ifndef UTILS_H_
#define UTILS_H_

#include <barrier>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
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
