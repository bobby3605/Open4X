#include "vulkan_window.hpp"

VulkanWindow::VulkanWindow(int w, int h, std::string name) : width{w}, height{h}, windowName{name} { initWindow(); }

VulkanWindow::~VulkanWindow() {
  glfwDestroyWindow(window);
  glfwTerminate();
}

void VulkanWindow::framebufferResizeCallback(GLFWwindow *window, int width, int height) {
  auto app = reinterpret_cast<VulkanWindow *>(glfwGetWindowUserPointer(window));
  app->framebufferResized = true;
}

void VulkanWindow::initWindow() {
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
  glfwSetWindowUserPointer(window, this);
  glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}