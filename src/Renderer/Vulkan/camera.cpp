#include "camera.hpp"
#include "window.hpp"
#include <glm/gtx/string_cast.hpp>
#include <iostream>

Camera::Camera() {}

void Camera::update_transform(float frame_time) {
    glm::vec3 rotate{0};
    float speed_up = 1;
    if (glfwGetKey(Window::window->glfw_window(), keys.yaw_right) == GLFW_PRESS)
        rotate.x += 1.f;
    if (glfwGetKey(Window::window->glfw_window(), keys.yaw_left) == GLFW_PRESS)
        rotate.x -= 1.f;
    if (glfwGetKey(Window::window->glfw_window(), keys.pitch_up) == GLFW_PRESS)
        rotate.y += 1.f;
    if (glfwGetKey(Window::window->glfw_window(), keys.pitch_down) == GLFW_PRESS)
        rotate.y -= 1.f;
    if (glfwGetKey(Window::window->glfw_window(), keys.roll_right) == GLFW_PRESS)
        rotate.z += 1.f;
    if (glfwGetKey(Window::window->glfw_window(), keys.roll_left) == GLFW_PRESS)
        rotate.z -= 1.f;
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
