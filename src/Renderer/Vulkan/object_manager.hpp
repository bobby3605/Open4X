#ifndef OBJECT_MANAGER_H_
#define OBJECT_MANAGER_H_

#include "object.hpp"
#include <glm/fwd.hpp>
#include <string>
#include <unordered_map>

class ObjectManager {
  public:
    ~ObjectManager();
    Object* add_object(std::string name, Model* model);
    void remove_object(std::string name);
    Object* get_object(std::string name);
    void refresh_instance_data();

  private:
    std::unordered_map<std::string, Object*> _objects;
    safe_queue<Object*> invalid_callback;
};

#endif // OBJECT_MANAGER_H_
