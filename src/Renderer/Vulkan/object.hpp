#ifndef OBJECT_H_
#define OBJECT_H_
#include "../../utils.hpp"
#include "model.hpp"
#include <glm/gtx/quaternion.hpp>

class Object {
  public:
    Object();
    Object(Model* model, safe_queue<Object*>* invalid_callback);

    glm::vec3 const& position() { return _position; }
    glm::quat const& rotation() { return _rotation; }
    glm::vec3 const& scale() { return _scale; }

    void position(glm::vec3 const& new_position);
    void rotation(glm::quat const& new_rotation);
    void scale(glm::vec3 const& new_scale);
    void register_invalid_matrices();
    void refresh_instance_data();

  protected:
    Model* _model;
    glm::vec3 _position{1.0f, 1.0f, 1.0f};
    glm::quat _rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 _scale{1.0f, 1.0f, 1.0f};
    glm::mat4 _object_matrix;
    // NOTE:
    // Set to false so that register_invalid_matrices() works in the constructor
    bool _instance_data_invalid = false;
    std::vector<uint32_t> _instance_ids;

    safe_queue<Object*>* _invalid_callback = nullptr;
};

#endif // OBJECT_H_
