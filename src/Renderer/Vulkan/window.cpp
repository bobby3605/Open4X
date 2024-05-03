#include "window.hpp"

Window* Window::window;
Window::Window(uint32_t width, uint32_t height, std::string name) : _width{width}, _height{height}, _name{name} {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    _window = glfwCreateWindow(width, height, _name.c_str(), nullptr, nullptr);
    glfwSetWindowUserPointer(glfw_window(), this);
    window = this;
}

Window::~Window() {
    glfwDestroyWindow(glfw_window());
    glfwTerminate();
}
