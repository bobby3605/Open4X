#include "utils.hpp"

ThreadPool::ThreadPool(size_t const& num_threads, std::function<void(size_t thread_id)> const& work_task)
    : _num_threads(num_threads), _thread_semaphore(0), _thread_barrier(_num_threads + 1) {
    for (size_t thread_id = 0; thread_id < _num_threads; ++thread_id) {
        _threads.emplace_back([&, work_task, thread_id] {
            while (true) {
                _thread_semaphore.acquire();
                if (_stop_threads) {
                    return;
                }
                work_task(thread_id);
                _thread_barrier.arrive_and_wait();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    // wake up all threads and cause them to exit
    _stop_threads = true;
    _thread_semaphore.release(_num_threads);
    for (auto& thread : _threads) {
        thread.join();
    }
}

void ThreadPool::run() {
    // run work_task once on each thread and wait for completion
    _thread_semaphore.release(_num_threads);
    _thread_barrier.arrive_and_wait();
}
