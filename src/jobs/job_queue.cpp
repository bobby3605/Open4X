#include "job_queue.hpp"
#include <atomic>
// https://blog.molecular-matters.com/2015/08/24/job-system-2-0-lock-free-work-stealing-part-1-basics/

JobQueue::JobQueue(size_t const& max_jobs) : jobs_mask(max_jobs - 1u), _jobs(max_jobs) {}

void JobQueue::push(Job* job) {
    size_t bottom = _bottom.load(std::memory_order_seq_cst);
    _jobs[bottom & jobs_mask] = job;
    _bottom.store(bottom + 1, std::memory_order_seq_cst);
    _bottom.notify_one();
}

Job* JobQueue::steal() {
    size_t top = _top.load(std::memory_order_seq_cst);
    size_t bottom = _bottom.load(std::memory_order_seq_cst);

    if (top < bottom && bottom != -1u) {
        Job* job = _jobs[top & jobs_mask];
        if (_top.compare_exchange_strong(top, top + 1, std::memory_order_seq_cst)) {
            return job;
        }
    }
    return nullptr;
}

Job* JobQueue::pop() {
    if (_top.load(std::memory_order_seq_cst) == _bottom.load(std::memory_order_seq_cst)) {
        return nullptr;
    }
    size_t bottom = _bottom.fetch_sub(1, std::memory_order_seq_cst) - 1;
    size_t top = _top.load(std::memory_order_seq_cst);

    if (top <= bottom) {
        Job* job = _jobs[bottom & jobs_mask];
        if (top != bottom) {
            return job;
        } else if (_top.compare_exchange_strong(top, top + 1, std::memory_order_seq_cst)) {
            _bottom = top + 1;
            return job;
        }
    }
    return nullptr;
}

void JobQueue::wait_nonempty() const {
    size_t not_equal = _top.load(std::memory_order_seq_cst);
    // when _bottom == _top, then the size is 0
    _bottom.wait(not_equal, std::memory_order_seq_cst);
}
