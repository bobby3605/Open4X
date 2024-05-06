#include "object.hpp"
#include <glm/gtx/quaternion.hpp>

Object::Object() {}

Object::Object(Model* model, safe_queue<Object*>* invalid_callback, Buffer* instance_data_buffer)
    : _model(model), _invalid_callback(invalid_callback), _instance_data_buffer(instance_data_buffer) {
    // pre allocate capacity
    _instance_data.reserve(model->model_matrices_size());
    _instance_data_buffer->alloc(sizeof(InstanceData) * _instance_data.capacity(), _instance_data_alloc, _instance_data_offset);
}

Object::~Object() {
    // TODO
    // could be nullptr because of Camera,
    // not sure if Camera calls this destructor...
    // probably doesn't
    if (_instance_data_buffer != nullptr)
        _instance_data_buffer->free(_instance_data_alloc);
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

bool Object::update_instance_data() {
    if (_object_matrix_invalid) {
        _object_matrix = glm::translate(glm::mat4(), position());
        _object_matrix = _object_matrix * glm::toMat4(rotation());
        _object_matrix = _object_matrix * glm::scale(_object_matrix, scale());
        _model->load_instance_data(_object_matrix, _instance_data);
        _object_matrix_invalid = false;
        return true;
    }
    return false;
}

void Object::register_invalid_matrices() {
    // Only register invalid once per update_instance_data call
    if (_object_matrix_invalid == false) {
        _object_matrix_invalid = true;
        _invalid_callback->push(this);
    }
}
