#include "job_queue.hpp"
// https://blog.molecular-matters.com/2015/08/24/job-system-2-0-lock-free-work-stealing-part-1-basics/

JobQueue::JobQueue(size_t const& max_jobs) : _queue(max_jobs) {}

void JobQueue::push(Job* job) {
    size_t bottom = _bottom.load(std::memory_order_seq_cst);
    _queue[bottom & (_queue.size() - 1u)] = job;
    _bottom.store(bottom + 1, std::memory_order_seq_cst);
}

Job* JobQueue::pop() {
    size_t bottom = _bottom.load(std::memory_order_seq_cst) - 1;
    _bottom.store(bottom, std::memory_order_seq_cst);

    size_t top = _top;
    if (top <= bottom) {
        Job* job = _queue[bottom];
        if (top != bottom) {
            return job;
        }

        if (_top.compare_exchange_strong(top, top + 1, std::memory_order_seq_cst) != top) {
            job = nullptr;
        }

        _bottom = top + 1;
        return job;
    } else {
        _bottom.store(top, std::memory_order_seq_cst);
        return nullptr;
    }
}

Job* JobQueue::steal() {
    size_t top = _top.load(std::memory_order_seq_cst);
    size_t bottom = _bottom.load(std::memory_order_seq_cst);

    if (top < bottom) {
        Job* job = _queue[top & (_queue.size() - 1u)];

        if (_top.compare_exchange_strong(top, top + 1, std::memory_order_seq_cst) != top) {
            return nullptr;
        }
        return job;
    } else {
        return nullptr;
    }
}
