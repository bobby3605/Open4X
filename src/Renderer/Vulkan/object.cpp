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

Object::Object(Model* model, safe_vector<Object*>& invalid_objects) : _model(model), _invalid_objects(invalid_objects) {
    model->add_instance(_instances);
    register_invalid_matrices();
}

Object::~Object() {
    if (_model != nullptr) {
        _model->remove_instance(_instances);
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
        if (_model != nullptr)
            _model->write_instance_data(_object_matrix, _instances);
        _instance_data_invalid = false;
        _t_invalid = false;
        _rs_invalid = false;
    }
}

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
