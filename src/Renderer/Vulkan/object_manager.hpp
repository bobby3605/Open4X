#ifndef OBJECT_MANAGER_H_
#define OBJECT_MANAGER_H_

#include "../../utils.hpp"
#include "object.hpp"
#include <barrier>
#include <glm/fwd.hpp>
#include <semaphore>
#include <string>
#include <unordered_map>

class ObjectManager {
  public:
    ObjectManager();
    ~ObjectManager();
    Object* add_object(std::string name, Model* model);
    void remove_object(std::string name);
    Object* get_object(std::string name);
    size_t object_count();
    void refresh_invalid_objects();
    void preallocate(size_t count);

  private:
    std::unordered_map<std::string, Object*> _objects;
    std::vector<Object*> _invalid_objects;
    std::vector<std::thread> _threads;
    unsigned int _num_threads;
    std::counting_semaphore<> _thread_semaphore;
    std::barrier<> _thread_barrier;
    std::atomic<bool> _stop_threads = false;
    std::vector<VectorSlice> _vector_slices;
};

#endif // OBJECT_MANAGER_H_
