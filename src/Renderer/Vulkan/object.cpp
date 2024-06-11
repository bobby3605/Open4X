#include "object.hpp"
#include "common.hpp"
#include <glm/gtx/transform.hpp>
#include <vulkan/vulkan_core.h>

Object::Object() { register_invalid_matrices(); }

Object::Object(Model* model, std::vector<Object*>* invalid_objects, std::mutex* invalid_objects_lock)
    : _model(model), _invalid_objects(invalid_objects), _invalid_objects_lock(invalid_objects_lock) {
    model->add_instance(_instances);
    register_invalid_matrices();
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

void Object::scale(glm::vec3 const& new_scale) {
    _scale = new_scale;
    _rs_invalid = true;
    register_invalid_matrices();
}

void Object::refresh_instance_data() {
    if (_instance_data_invalid) {
        if (_rs_invalid) {
            fast_r_matrix(_rotation, _object_matrix);
            fast_s_matrix(_scale, _object_matrix);
        }
        if (_t_invalid) {
            fast_t_matrix(_position, _object_matrix);
        }
        if (_model != nullptr)
            _model->write_instance_data(_object_matrix, _instances);
        _instance_data_invalid = false;
        _t_invalid = false;
        _rs_invalid = false;
    }
}

// TODO
// Maybe get rid of this and call update_instance_data() on all objects
// It's probably better to have it than not though
void Object::register_invalid_matrices() {
    // Only register invalid once per update_instance_data call
    if (_instance_data_invalid == false) {
        _instance_data_invalid = true;
        // NOTE:
        // Camera sets _invalid_objects to nullptr
        if (_invalid_objects != nullptr) {
            // TODO
            // remove this lock
            std::unique_lock<std::mutex> lock(*_invalid_objects_lock);
            _invalid_objects->push_back(this);
        }
    }
}
