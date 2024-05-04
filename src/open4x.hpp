#ifndef OPEN4X_H_
#define OPEN4X_H_

#include "Renderer/Vulkan/model_manager.hpp"
#include "Renderer/Vulkan/rendergraph.hpp"
#include "Renderer/Vulkan/window.hpp"
#include "Vulkan/common.hpp"
#include "Vulkan/vulkan_device.hpp"
#include "Vulkan/vulkan_model.hpp"
#include "Vulkan/vulkan_object.hpp"
#include "Vulkan/vulkan_objects.hpp"
#include "Vulkan/vulkan_window.hpp"
#include "glTF/GLTF.hpp"
#include <chrono>
#include <memory>

class Open4X {
  public:
    Open4X();
    ~Open4X();
    void run();

  private:
    VulkanObject* camera;

    VulkanWindow* vulkanWindow;
    std::shared_ptr<VulkanDevice> vulkanDevice;
    //    VulkanRenderer* vulkanRenderer;

    std::shared_ptr<VulkanObjects> objects;

    std::chrono::system_clock::time_point creationTime;

    std::shared_ptr<Settings> settings;
    void loadSettings();

    // New Renderer
    ModelManager* _model_manager;
    RenderGraph* rg;
};

#endif // OPEN4X_H_
