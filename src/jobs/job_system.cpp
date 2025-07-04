#include "job_system.hpp"
#include <cmath>

JobSystem::JobSystem(size_t const& worker_count, size_t const& log2_jobs) {
    job_queues.reserve(worker_count);
    workers.reserve(worker_count);
    size_t max_jobs = std::pow(2, log2_jobs);
    for (size_t i = 0; i < worker_count; ++i) {
        job_queues.push_back(new JobQueue(max_jobs));
        workers.push_back(new Worker(job_queues, i));
    }
}

JobSystem::~JobSystem() { stop(); }

void JobSystem::stop() {
    if (stopped)
        return;
    for (size_t i = 0; i < workers.size(); ++i) {
        Worker* worker = workers[i];
        worker->stop = true;

        JobQueue* queue = job_queues[i];
        queue->_bottom = -1;
        queue->_bottom.notify_one();

        worker->background_thread.join();
        delete worker;
    }
    // delete queues after all workers have exited
    for (auto& queue : job_queues) {
        delete queue;
    }
    stopped = true;
}
