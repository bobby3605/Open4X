#ifndef OBJECT_MANAGER_H_
#define OBJECT_MANAGER_H_

#include "object.hpp"
#include <glm/fwd.hpp>
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

  private:
    std::unordered_map<std::string, Object*> _objects;
    safe_queue_external_sync<Object*> invalid_callback;
    std::vector<std::thread> _threads;
    std::atomic<bool> _stop_threads = false;
    std::mutex _invalid_callback_mutex;
    std::condition_variable _invalid_callback_cond;
};

#endif // OBJECT_MANAGER_H_
