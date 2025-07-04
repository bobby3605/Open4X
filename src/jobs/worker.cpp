#include "worker.hpp"

Worker::Worker(std::vector<JobQueue*> const& job_queues, size_t queue_idx)
    : _job_queue(job_queues[queue_idx]), _jobs(_job_queue->size()), _distribution(0, _job_queue->size()), _job_queues(job_queues),
      background_thread(&Worker::main, this) {}

void Worker::main() {
    while (!stop) {
        Job* job = get_job();
        if (job) {
            job->execute();
        }
    }
}

Job* Worker::get_job() {
    Job* job = _job_queue->pop();
    if (!job) {
        JobQueue* rand_queue = _job_queues[_distribution(_mt)];
        if (rand_queue && rand_queue != _job_queue) {
            job = rand_queue->steal();
        }
        if (job) {
            return job;
        }
        std::this_thread::yield();
        return nullptr;
    }
    return job;
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

Job* Worker::allocate_job() {
    const size_t index = _allocated_jobs++;
    return &_jobs[index & (_job_queue->size() - 1)];
}

void Worker::submit(Job* job) { _job_queue->push(job); }

void Worker::wait(const Job* job) {
    while (!job->completed()) {
        Job* next_job = get_job();
        if (next_job) {
            next_job->execute();
        }
    }
}
