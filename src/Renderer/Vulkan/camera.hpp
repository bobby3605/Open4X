#ifndef CAMERA_H_
#define CAMERA_H_
#include "object.hpp"
#include <GLFW/glfw3.h>

class Camera : public Object {
  public:
    Camera();
    void update_transform(float frame_time);
    glm::mat4 const& proj_view();

    inline static const glm::vec3 up_vector = glm::vec3(0.f, -1.0f, 0.f);
    inline static const glm::vec3 forward_vector = glm::vec3(0.0f, 0.0f, 1.0f);
    inline static const glm::vec3 right_vector = glm::vec3(1.0f, 0.0, 0.0f);

  private:
    glm::mat4 _proj{0.0f};
    glm::mat4 _proj_view;
    struct KeyMappings {
        static const inline int move_left = GLFW_KEY_A;
        static const inline int move_right = GLFW_KEY_D;
        static const inline int move_forward = GLFW_KEY_W;
        static const inline int move_backward = GLFW_KEY_S;
        static const inline int move_up = GLFW_KEY_Q;
        static const inline int move_down = GLFW_KEY_E;
        static const inline int yaw_left = GLFW_KEY_LEFT;
        static const inline int yaw_right = GLFW_KEY_RIGHT;
        static const inline int pitch_up = GLFW_KEY_DOWN;
        static const inline int pitch_down = GLFW_KEY_UP;
        static const inline int roll_left = GLFW_KEY_LEFT_CONTROL;
        static const inline int roll_right = GLFW_KEY_RIGHT_CONTROL;
        static const inline int speed_up = GLFW_KEY_LEFT_SHIFT;
        static const inline int slow_down = GLFW_KEY_LEFT_ALT;
    };
    static constexpr KeyMappings keys{};

    // FIXME:
    // Set these dynamically
    float vertical_fov = 45.0f;
    float aspect_ratio = 640.0f / 480.0f;
    float near = 0.0001f;
    float far = 1000.0f;

    float move_speed{6.0f};
    float look_speed{2.0f};

    // FIXME:
    // this has to be set because Object takes a reference to this
    safe_vector<Object*> _invalid_objects;
};

#endif // CAMERA_H_
