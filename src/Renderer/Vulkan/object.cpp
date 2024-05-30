#include "object.hpp"
#include <glm/gtx/quaternion.hpp>
#include <vulkan/vulkan_core.h>

Object::Object(Model* model, safe_queue<Object*>* invalid_callback) : _model(model), _invalid_callback(invalid_callback) {
    register_invalid_matrices();
    _instance_ids = model->add_instance();
}

void Object::position(glm::vec3 const& new_position) {
    _position = new_position;
    register_invalid_matrices();
}
void Object::rotation(glm::quat const& new_rotation) {
    _rotation = new_rotation;
    register_invalid_matrices();
}
void Object::scale(glm::vec3 const& new_scale) {
    _scale = new_scale;
    register_invalid_matrices();
}

void Object::refresh_instance_data() {
    if (_instance_data_invalid) {
        _object_matrix = glm::translate(glm::mat4(), position());
        _object_matrix = _object_matrix * glm::toMat4(rotation());
        _object_matrix = _object_matrix * glm::scale(_object_matrix, scale());
        _model->write_instance_data(_object_matrix, _instance_ids);
        _instance_data_invalid = false;
    }
}

// TODO
// Maybe get rid of this and call update_instance_data() on all objects
// It's probably better to have it than not though
void Object::register_invalid_matrices() {
    // Only register invalid once per update_instance_data call
    if (_instance_data_invalid == false) {
        _instance_data_invalid = true;
        _invalid_callback->push(this);
    }
}
