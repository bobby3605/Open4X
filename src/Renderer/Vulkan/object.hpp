#ifndef OBJECT_H_
#define OBJECT_H_
#include "../../utils.hpp"
#include "../Allocator/sub_allocator.hpp"
#include "model.hpp"

class Object {
  public:
    Object();
    Object(Model* model, safe_queue<Object*>* invalid_callback, StackAllocator<GPUAllocator>* instances_sub_allocator);
    ~Object();

    glm::vec3 const& position() { return _position; }
    glm::quat const& rotation() { return _rotation; }
    glm::vec3 const& scale() { return _scale; }

    void position(glm::vec3 const& new_position);
    void rotation(glm::quat const& new_rotation);
    void scale(glm::vec3 const& new_scale);
    void register_invalid_matrices();

    // returns true if _instance_data was changed,
    // otherwise false
    bool update_instance_data();
    std::vector<InstanceData> const& instance_data() { return _instance_data; }
    std::vector<SubAllocation> const& instance_data_allocs() { return _instance_data_allocs; }

  private:
    Model* _model;
    glm::vec3 _position;
    glm::quat _rotation;
    glm::vec3 _scale;
    glm::mat4 _object_matrix;
    bool _object_matrix_invalid = true;
    std::vector<InstanceData> _instance_data;
    StackAllocator<GPUAllocator>* _instances_sub_allocator = nullptr;
    std::vector<SubAllocation> _instance_data_allocs;

    safe_queue<Object*>* _invalid_callback;
};

#endif // OBJECT_H_
