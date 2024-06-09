#ifndef UTILS_H_
#define UTILS_H_

#include <condition_variable>
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

// partially apply tail arguments
// returns a std::function<ReturnType(FirstArgumentType)>
// could probably be made more generic
template <typename F, typename... Args> auto partial(F f, Args... tail_args) {
    return [=](auto head_arg) { return f(head_arg, tail_args...); };
}

#endif // UTILS_H_
