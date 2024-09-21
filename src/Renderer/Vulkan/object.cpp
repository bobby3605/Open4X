#include "object.hpp"
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtx/transform.hpp>
#include <vulkan/vulkan_core.h>

// FIXME:
// since these are references, they have to be initialized,
// but they'll never be used if model is nullptr
// references should be faster than pointers, but it might not be worth it
// const pointer might get optimized to a reference, but without this issue
Object::Object(safe_vector<Object*>& invalid_objects) : _model(nullptr), _invalid_objects(invalid_objects) { register_invalid_matrices(); }

Object::Object(Model* model, safe_vector<Object*>& invalid_objects, LinearAllocator<GPUAllocation>* object_instance_ids_allocator,
               ContiguousFixedAllocator<GPUAllocation>* object_culling_data_allocator)
    : _model(model), _invalid_objects(invalid_objects), _object_instance_ids_allocator(object_instance_ids_allocator),
      _object_culling_data_allocator(object_culling_data_allocator) {
    model->add_instance(_instances);
    register_invalid_matrices();
    std::vector<uint32_t> instance_offsets;
    instance_offsets.reserve(_instances.capacity());
    for (size_t i = 0; i < _instances.capacity(); ++i) {
        InstanceAllocPair const& instance_alloc = _instances[i];
        size_t index_offset = instance_alloc.index->offset() / instance_alloc.index->size();
        // add firstInstance to the offset to make it global
        index_offset += instance_alloc.index->parent()->offset() / sizeof(uint32_t);
        instance_offsets.push_back(index_offset);
    }
    if (instance_offsets.size() == 0) {
        std::cout << "0 instance offsets size: " << _model->path() << std::endl;
        std::cout << "instances capacity: " << _instances.capacity() << std::endl;
    }
    _objects_instance_ids_allocation = object_instance_ids_allocator->alloc(instance_offsets.size() * sizeof(uint32_t));
    // FIXME:
    // update this when instance index alloc gets pop and swapped as the last block
    _objects_instance_ids_allocation->write(instance_offsets.data());

    _object_culling_data_alloc = _object_culling_data_allocator->alloc();
}

Object::~Object() {
    if (_model != nullptr) {
        _model->remove_instance(_instances);
        _object_instance_ids_allocator->free(_objects_instance_ids_allocation);
        _object_culling_data_allocator->pop_and_swap(_object_culling_data_alloc);
    }
    for (size_t i = 0; i < _invalid_objects.size(); ++i) {
        if (_invalid_objects.at(i) == this) {
            _invalid_objects.erase(i);
            break;
        }
    }
}

void Object::position(glm::vec3 const& new_position) {
    _position = new_position;
    _t_invalid = true;
    register_invalid_matrices();
}

void Object::rotation(glm::quat const& new_rotation) {
    _rotation = new_rotation;
    _rs_invalid = true;
    register_invalid_matrices();
}

void Object::rotation_euler(float pitch, float yaw, float roll) { rotation(glm::vec3{pitch, yaw, roll} * (glm::pi<float>() / 180)); }

void Object::scale(glm::vec3 const& new_scale) {
    _scale = new_scale;
    _rs_invalid = true;
    register_invalid_matrices();
}

void Object::refresh_culling_data() {
    ObjectCullData data{};
    // TODO:
    // Support animations
    data.obb = _model->aabb().toOBB(rotation(), scale());
    glm::vec3 position_offset = _model->aabb().centerpoint() * scale();
    data.obb.center += position() - position_offset;

    data.instance_count = _instances.capacity();
    // FIXME:
    // pop and swap update
    data.instances_offset = _objects_instance_ids_allocation->offset() / sizeof(uint32_t);
    _object_culling_data_alloc->write(&data);
}

void Object::refresh_instance_data() {
    if (_instance_data_invalid) {
        if (_rs_invalid) {
            fast_r_matrix(_rotation, _object_matrix);
            fast_s_matrix(_scale, _object_matrix);
        }
        if (_t_invalid) {
            glm::vec3 position_offset{0.0f, 0.0f, 0.0f};
            if (_model != nullptr) {
                // NOTE:
                // Correct the position by the centerpoint and scale of the model
                position_offset = _model->aabb().centerpoint() * scale();
            }
            fast_t_matrix(_position - position_offset, _object_matrix);
        }
        if (_model != nullptr) {
            _model->write_instance_data(_object_matrix, _instances);
            refresh_culling_data();
        }

        _instance_data_invalid = false;
        _t_invalid = false;
        _rs_invalid = false;
    }
}

void Object::refresh_animations() { _model->animate(_instances); }

void Object::register_invalid_matrices() {
    // Only register invalid once per update_instance_data call
    if (_instance_data_invalid == false) {
        _instance_data_invalid = true;
        // NOTE:
        // Camera sets _model to nullptr
        if (_model != nullptr) {
            _invalid_objects.push_back(this);
        }
    }
}
