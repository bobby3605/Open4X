#include "object.hpp"
#include <glm/gtx/transform.hpp>
#include <vulkan/vulkan_core.h>

Object::Object() { register_invalid_matrices(); }

Object::Object(Model* model, std::vector<Object*>* invalid_callback) : _model(model), _invalid_objects(invalid_callback) {
    _instances = model->add_instance();
    register_invalid_matrices();
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
        // Manually copying and setting object matrix because multiplying mat4 is slow
        // translation
        _object_matrix[3][0] = _position.x;
        _object_matrix[3][1] = _position.y;
        _object_matrix[3][2] = _position.z;
        // rotation
        glm::mat3 rotation_3x3 = glm::toMat3(rotation());
        _object_matrix[0][0] = rotation_3x3[0][0];
        _object_matrix[0][1] = rotation_3x3[0][1];
        _object_matrix[0][2] = rotation_3x3[0][2];
        _object_matrix[1][0] = rotation_3x3[1][0];
        _object_matrix[1][1] = rotation_3x3[1][1];
        _object_matrix[1][2] = rotation_3x3[1][2];
        _object_matrix[2][0] = rotation_3x3[2][0];
        _object_matrix[2][1] = rotation_3x3[2][1];
        _object_matrix[2][2] = rotation_3x3[2][2];
        // scale
        _object_matrix[0][0] *= _scale.x;
        _object_matrix[1][1] *= _scale.y;
        _object_matrix[2][2] *= _scale.z;
        // TODO
        // Use a 4x3 matrix
        _object_matrix[0][3] = 0;
        _object_matrix[1][3] = 0;
        _object_matrix[2][3] = 0;
        _object_matrix[3][3] = 1;

        if (_model != nullptr)
            _model->write_instance_data(_object_matrix, _instances);
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
        // NOTE:
        // Camera sets _invalid_objects to nullptr
        if (_invalid_objects != nullptr) {
            _invalid_objects->push_back(this);
        }
    }
}
