#ifndef OBJECT_MANAGER_H_
#define OBJECT_MANAGER_H_

#include "object.hpp"
#include <string>
#include <unordered_map>

class ObjectManager {
  public:
    ObjectManager();
    ~ObjectManager();
    void add_object(std::string name, Model* model);
    void remove_object(std::string name);

  private:
    std::unordered_map<std::string, Object*> _objects;
};

#endif // OBJECT_MANAGER_H_
