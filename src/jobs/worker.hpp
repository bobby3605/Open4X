#ifndef WORKER_H_
#define WORKER_H_
#include "job_queue.hpp"
#include <thread>

class Worker {
    JobQueue* _job_queue;

  public:
    Worker(std::vector<JobQueue*> const& job_queues, size_t queue_idx);
    Job* create_job(Job::JobFunction function, void* data, Job* parent = nullptr);
    void submit(Job* job);
    bool stop = false;

  private:
    void main();
    void run_next_job();
    Job* allocate_job();
    Job* get_job();
    size_t _allocated_jobs;
    std::vector<Job> _jobs;
    std::vector<JobQueue*> const& _job_queues;
    size_t _previous_queue_stolen;

  public:
    std::thread background_thread;
};

#endif // WORKER_H_
