#ifndef OPEN4X_H_
#define OPEN4X_H_

#include "Vulkan/vulkan_device.hpp"
#include "Vulkan/vulkan_model.hpp"
#include "Vulkan/vulkan_renderer.hpp"
#include "Vulkan/vulkan_window.hpp"

class Open4X {
public:
  Open4X();
  ~Open4X();
  void run();

private:
  VulkanWindow *vulkanWindow;
  VulkanDevice *vulkanDevice;
  VulkanRenderer *vulkanRenderer;
  VulkanModel *vulkanModel;
};

#endif // OPEN4X_H_
