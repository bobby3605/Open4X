#ifndef JOB_H_
#define JOB_H_
#include <atomic>

class Job {
  public:
    typedef void (*JobFunction)(void* const);
    JobFunction function;
    Job* parent = nullptr;
    std::atomic<size_t> unfinished_dependencies;
    void execute();
    void finish();
    bool completed() const { return unfinished_dependencies == 0; }
    void* data;
};

#endif // JOB_H_
