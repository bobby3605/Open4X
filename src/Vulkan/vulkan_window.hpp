#ifndef VULKAN_WINDOW_H_
#define VULKAN_WINDOW_H_

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>
#include <vulkan/vulkan.h>

class VulkanWindow {
  public:
    VulkanWindow(int w, int h, std::string name);
    ~VulkanWindow();

    bool framebufferResized = false;
    GLFWwindow* getGLFWwindow() { return window; }
    bool shouldClose() { return glfwWindowShouldClose(window); }
    VkExtent2D getExtent() {
        return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
    }

  private:
    static void framebufferResizeCallback(GLFWwindow* window, int width,
                                          int height);
    void initWindow();

    GLFWwindow* window;
    int width;
    int height;
    std::string windowName;
};

#endif // VULKAN_WINDOW_H_
