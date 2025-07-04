#ifndef WORKER_H_
#define WORKER_H_
#include "job_queue.hpp"
#include <random>
#include <thread>

class Worker {
    JobQueue* _job_queue;

  public:
    Worker(std::vector<JobQueue*> const& job_queues, size_t queue_idx);
    Job* allocate_job();
    Job* create_job(Job::JobFunction function, void* data, Job* parent = nullptr);
    void wait(const Job* job);
    bool stop = false;

  private:
    void main();
    Job* get_job();
    void submit(Job* job);
    size_t _allocated_jobs;
    std::vector<Job> _jobs;
    std::mt19937 _mt;
    std::uniform_int_distribution<size_t> _distribution;
    std::vector<JobQueue*> const& _job_queues;

  public:
    std::thread background_thread;
};

#endif // WORKER_H_
