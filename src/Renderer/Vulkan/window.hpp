#ifndef WINDOW_H_
#define WINDOW_H_

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>
#include <vulkan/vulkan.hpp>

class Window {
  public:
    Window(uint32_t width, uint32_t height, std::string name);
    ~Window();

    static Window* window;

    GLFWwindow* glfw_window() { return _window; }
    std::string name() { return _name; }
    bool should_close() { return glfwWindowShouldClose(glfw_window()); }
    VkExtent2D extent();

  private:
    GLFWwindow* _window;
    uint32_t _width;
    uint32_t _height;
    std::string _name;
};

#endif // WINDOW_H_
