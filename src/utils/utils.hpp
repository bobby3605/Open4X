#ifndef UTILS_H_
#define UTILS_H_

#include <barrier>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>

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
    std::vector<T>& _vector;
    std::vector<VectorSlice> _vector_slices;
    std::function<void(T& t)> _work_task;
    ThreadPool _thread_pool;

  public:
    VectorSlicer(std::vector<T>& vector, size_t const& num_threads, std::function<void(T& t)> const& work_task)
        : _vector(vector), _work_task(work_task), _thread_pool(num_threads, [&](size_t thread_id) {
              VectorSlice& slice = _vector_slices[thread_id];
              size_t end_offset = slice.offset + slice.size;
              for (size_t i = slice.offset; i < end_offset; ++i) {
                  _work_task(_vector[i]);
              }
          }) {
        _vector_slices.reserve(num_threads);
    }
    void run() {
        size_t vector_size = _vector.size();
        size_t num_threads = _thread_pool.num_threads();

        // run on this thread if the thread pool would be too much overhead
        if (vector_size < num_threads || num_threads == 0) {
            for (auto& t : _vector) {
                _work_task(t);
            }
        } else {
            size_t slice_size = vector_size / num_threads;
            size_t remainder = vector_size % num_threads;
            size_t curr_offset = 0;
            for (size_t i = 0; i < num_threads; ++i) {
                _vector_slices[i].size = slice_size;
                _vector_slices[i].offset = curr_offset;
                curr_offset += slice_size;
                if (i == num_threads - 1) {
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

#endif // UTILS_H_
