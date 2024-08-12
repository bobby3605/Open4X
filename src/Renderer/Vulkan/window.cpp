#include "window.hpp"
#include <GLFW/glfw3.h>

Window* Window::window;
Window::Window(uint32_t width, uint32_t height, std::string name) : _width{width}, _height{height}, _name{name} {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    _window = glfwCreateWindow(width, height, _name.c_str(), nullptr, nullptr);
    glfwSetWindowUserPointer(glfw_window(), this);
    window = this;
}

VkExtent2D Window::extent() {
    VkExtent2D ext;
    glfwGetFramebufferSize(glfw_window(), reinterpret_cast<int*>(&ext.width), reinterpret_cast<int*>(&ext.height));
    return ext;
}

Window::~Window() {
    glfwDestroyWindow(glfw_window());
    glfwTerminate();
}
