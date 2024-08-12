#include "camera.hpp"
#include "window.hpp"
#include <glm/gtx/string_cast.hpp>
#include <iostream>

Camera::Camera(float vertical_fov, float aspect_ratio, float near, float far) : Object(_invalid_objects) {
    update_projection(vertical_fov, aspect_ratio, near, far);
}

void Camera::update_projection(float vertical_fov, float aspect_ratio, float near, float far){
    _instance_data_invalid = true;
    // perspective projection
    assert(glm::abs(aspect_ratio - std::numeric_limits<float>::epsilon()) > 0.0f);
    const float tanHalfFovy = tan(glm::radians(vertical_fov) / 2.f);
    _proj[0][0] = 1.f / (aspect_ratio * tanHalfFovy);
    _proj[1][1] = 1.f / (tanHalfFovy);
    _proj[2][2] = far / (far - near);
    _proj[2][3] = 1.f;
    _proj[3][2] = -(far * near) / (far - near);
}

glm::mat4 const& Camera::proj_view() {
    // TODO
    // This probably gets called every frame,
    // so caching is pointless
    if (_instance_data_invalid) {
        refresh_instance_data();
        _proj_view = _proj * glm::inverse(_object_matrix);
    }
    return _proj_view;
}

// Define roll/pitch/yaw ordering
// vulkan uses inverted-y right handed coordinates
#define pitch x
#define yaw y
#define roll z

void Camera::update_transform(float frame_time) {
    glm::vec3 rotate{0};
    float speed_up = 1;
    if (glfwGetKey(Window::window->glfw_window(), keys.pitch_up) == GLFW_PRESS)
        rotate.pitch += 1.f;
    if (glfwGetKey(Window::window->glfw_window(), keys.pitch_down) == GLFW_PRESS)
        rotate.pitch -= 1.f;
    if (glfwGetKey(Window::window->glfw_window(), keys.yaw_right) == GLFW_PRESS)
        rotate.yaw += 1.f;
    if (glfwGetKey(Window::window->glfw_window(), keys.yaw_left) == GLFW_PRESS)
        rotate.yaw -= 1.f;
    if (glfwGetKey(Window::window->glfw_window(), keys.roll_right) == GLFW_PRESS)
        rotate.roll += 1.f;
    if (glfwGetKey(Window::window->glfw_window(), keys.roll_left) == GLFW_PRESS)
        rotate.roll -= 1.f;
    if (glfwGetKey(Window::window->glfw_window(), keys.speed_up) == GLFW_PRESS)
        speed_up = 2;
    if (glfwGetKey(Window::window->glfw_window(), keys.slow_down) == GLFW_PRESS)
        speed_up = 0.5;

    if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon()) {
        rotation(rotation() * glm::quat(speed_up * look_speed * frame_time * glm::normalize(rotate)));
    }

    const glm::vec3 forward_dir = rotation() * forward_vector;
    const glm::vec3 right_dir = rotation() * right_vector;
    const glm::vec3 up_dir = rotation() * up_vector;

    glm::vec3 move_dir{0.f};
    speed_up = 1;
    if (glfwGetKey(Window::window->glfw_window(), keys.move_forward) == GLFW_PRESS)
        move_dir += forward_dir;
    if (glfwGetKey(Window::window->glfw_window(), keys.move_backward) == GLFW_PRESS)
        move_dir -= forward_dir;
    if (glfwGetKey(Window::window->glfw_window(), keys.move_right) == GLFW_PRESS)
        move_dir += right_dir;
    if (glfwGetKey(Window::window->glfw_window(), keys.move_left) == GLFW_PRESS)
        move_dir -= right_dir;
    if (glfwGetKey(Window::window->glfw_window(), keys.move_up) == GLFW_PRESS)
        move_dir += up_dir;
    if (glfwGetKey(Window::window->glfw_window(), keys.move_down) == GLFW_PRESS)
        move_dir -= up_dir;
    if (glfwGetKey(Window::window->glfw_window(), keys.speed_up) == GLFW_PRESS)
        speed_up = 2.5;
    if (glfwGetKey(Window::window->glfw_window(), keys.slow_down) == GLFW_PRESS)
        speed_up = 0.4;

    if (glm::dot(move_dir, move_dir) > std::numeric_limits<float>::epsilon()) {
        position(position() + (speed_up * move_speed * frame_time * glm::normalize(move_dir)));
    }

    if (glfwGetKey(Window::window->glfw_window(), GLFW_KEY_SPACE))
        std::cout << "Postion: " << glm::to_string(position()) << std::endl << "Rotation: " << glm::to_string(rotation()) << std::endl;
}
