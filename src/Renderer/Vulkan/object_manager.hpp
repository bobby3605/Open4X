#ifndef OBJECT_MANAGER_H_
#define OBJECT_MANAGER_H_

#include "buffer.hpp"
#include "object.hpp"
#include <glm/fwd.hpp>
#include <string>
#include <unordered_map>

class ObjectManager {
  public:
    ObjectManager(Buffer* _instance_data_buffer);
    ~ObjectManager();
    Object* add_object(std::string name, Model* model);
    void remove_object(std::string name);
    Object* get_object(std::string name);
    void update_instance_data(glm::mat4*& dst);

  private:
    std::unordered_map<std::string, Object*> _objects;
    void upload_instance_data(VkDeviceSize const& offset, std::vector<InstanceData> const& instance_data);
    Buffer* _instance_data_buffer;
    safe_queue<Object*> invalid_callback;
};

#endif // OBJECT_MANAGER_H_
