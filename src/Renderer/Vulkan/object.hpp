#ifndef OBJECT_H_
#define OBJECT_H_
#include "model.hpp"
#include <glm/gtx/quaternion.hpp>

class Object {
  public:
    Object();
    Object(Model* model, std::vector<Object*>* invalid_objects);

    glm::vec3 const& position() { return _position; }
    glm::quat const& rotation() { return _rotation; }
    glm::vec3 const& scale() { return _scale; }

    void position(glm::vec3 const& new_position);
    void rotation(glm::quat const& new_rotation);
    void scale(glm::vec3 const& new_scale);
    void register_invalid_matrices();
    void refresh_instance_data();

  protected:
    Model* _model = nullptr;
    glm::vec3 _position{0.0f, 0.0f, 0.0f};
    glm::quat _rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 _scale{1.0f, 1.0f, 1.0f};
    glm::mat4 _object_matrix{1.0f};
    // NOTE:
    // Set to false so that register_invalid_matrices() works in the constructor
    bool _instance_data_invalid = false;
    std::vector<InstanceAllocPair> _instances;

    std::vector<Object*>* _invalid_objects = nullptr;

    bool _t_invalid = false;
    bool _rs_invalid = false;
};

#endif // OBJECT_H_
