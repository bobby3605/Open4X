#include "worker.hpp"

Worker::Worker(std::vector<JobQueue*> const& job_queues, size_t queue_idx)
    : _job_queue(job_queues[queue_idx]), _jobs(_job_queue->jobs_mask + 1), _job_queues(job_queues), _previous_queue_stolen(queue_idx),
      background_thread(&Worker::main, this) {}

void Worker::main() {
    while (!stop) {
        run_next_job();
    }
}

void Worker::run_next_job() {
    constexpr uint32_t spins = 64;
    constexpr uint32_t yields = 8;
    Job* job = nullptr;
    for (uint32_t i = 0; i < spins; ++i) {
        job = get_job();
        if (job) {
            return job->execute();
        }
    }
    for (uint32_t i = 0; i < yields; ++i) {
        job = get_job();
        if (job) {
            return job->execute();
        }
        std::this_thread::yield();
    }
    // NOTE: jobs should be distributed and not put on a single queue
    // It should only get to this point if it was unable to steal from any queue
    _job_queue->wait_nonempty();
}

Job* Worker::get_job() {
    Job* job = _job_queue->pop();
    if (job) {
        return job;
    }
    size_t count = 0;
    for (size_t i = _previous_queue_stolen; count < _job_queues.size(); i = (i + 1) % _job_queues.size()) {
        job = _job_queues[i]->steal();
        if (job) {
            _previous_queue_stolen = i;
            return job;
        }
        ++count;
    }
    return nullptr;
}

Job* Worker::create_job(Job::JobFunction function, void* data, Job* parent) {
    if (parent) {
        ++parent->unfinished_dependencies;
    }
    Job* job = allocate_job();
    job->function = function;
    job->parent = parent;
    job->unfinished_dependencies = 1;
    job->data = data;

    return job;
}

Job* Worker::allocate_job() { return &_jobs[_allocated_jobs++ & _job_queue->jobs_mask]; }

void Worker::submit(Job* job) { _job_queue->push(job); }
