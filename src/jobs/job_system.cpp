#include "job_system.hpp"

JobSystem::JobSystem(size_t const& worker_count, size_t const& log2_jobs) {
    job_queues.reserve(worker_count);
    workers.reserve(worker_count);
    size_t max_jobs = 1 << log2_jobs;
    for (size_t i = 0; i < worker_count; ++i) {
        job_queues.push_back(new JobQueue(max_jobs));
        workers.push_back(new Worker(job_queues, i));
    }
}

JobSystem::~JobSystem() { stop(); }

void JobSystem::stop() {
    for (auto& worker : workers) {
        worker->stop = true;
        worker->background_thread.join();
    }
}
