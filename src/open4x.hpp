#ifndef OPEN4X_H_
#define OPEN4X_H_

#include "Vulkan/vulkan_device.hpp"
#include "Vulkan/vulkan_model.hpp"
#include "Vulkan/vulkan_object.hpp"
#include "Vulkan/vulkan_renderer.hpp"
#include "Vulkan/vulkan_window.hpp"
#include "glTF/GLTF.hpp"

class Open4X {
  public:
    Open4X();
    ~Open4X();
    void run();

  private:
    VulkanObject* camera;

    VulkanWindow* vulkanWindow;
    VulkanDevice* vulkanDevice;
    VulkanRenderer* vulkanRenderer;
    VulkanModel* basicTriangleModel;
    VulkanModel* vikingRoomModel;
    VulkanModel* flatVaseModel;
    gltf::GLTF* basicTriangleGLTFModel;

    std::vector<VulkanModel*> vulkanModels;
    std::vector<gltf::GLTF*> gltfModels;
};

#endif // OPEN4X_H_
