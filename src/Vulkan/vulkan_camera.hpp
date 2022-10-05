#ifndef VULKAN_CAMERA_H_
#define VULKAN_CAMERA_H_

#include "vulkan_renderer.hpp"

class VulkanCamera {
    VulkanCamera(VulkanRenderer* renderer);

  private:
    VulkanRenderer* renderer;
};

#endif // VULKAN_CAMERA_H_
