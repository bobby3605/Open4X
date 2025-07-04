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
    size_t size() const { return _queue.size(); }

  private:
    std::atomic<size_t> _bottom = 0;
    std::atomic<size_t> _top = 0;
    std::vector<Job*> _queue;
};

#endif // JOB_QUEUE_H_
