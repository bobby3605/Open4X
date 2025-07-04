#ifndef JOB_QUEUE_H_
#define JOB_QUEUE_H_
#include "job.hpp"
#include <vector>

class JobQueue {
  public:
    JobQueue(size_t const& max_jobs);
    void push(Job* job);
    Job* pop();
    Job* steal();
    const size_t jobs_mask;
    void wait_nonempty() const;

  private:
    std::atomic<size_t> _top = 0;
    std::vector<Job*> _jobs;

  protected:
    std::atomic<size_t> _bottom = 0;
    friend class JobSystem;
};

#endif // JOB_QUEUE_H_
