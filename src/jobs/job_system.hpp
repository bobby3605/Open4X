#ifndef JOBS_H_
#define JOBS_H_
#include "job_queue.hpp"
#include "worker.hpp"
#include <cmath>

class JobSystem {
  public:
    JobSystem(size_t const& worker_count, size_t const& log2_jobs);
    ~JobSystem();
    std::vector<JobQueue*> job_queues;
    std::vector<Worker*> workers;
    void stop();
};

#endif // JOBS_H_
