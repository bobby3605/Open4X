#ifndef UTILS_H_
#define UTILS_H_

#include <condition_variable>
#include <mutex>
#include <queue>
template <typename T> class safe_queue {
  public:
    void push(T item) {
        std::unique_lock<std::mutex> lock(mutex);
        queue.push(item);
        cond.notify_one();
    }
    T pop() {
        std::unique_lock<std::mutex> lock(mutex);
        cond.wait(lock, [this]() { return !queue.empty(); });
        T item = queue.front();
        queue.pop();
        return item;
    }
    bool empty() {
        std::unique_lock<std::mutex> lock(mutex);
        return queue.empty();
    }

  private:
    std::queue<T> queue;
    std::mutex mutex;
    std::condition_variable cond;
};

#endif // UTILS_H_
