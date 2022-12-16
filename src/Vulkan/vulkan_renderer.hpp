#ifndef VULKAN_RENDERER_H_
#define VULKAN_RENDERER_H_

#include "vulkan_descriptors.hpp"
#include "vulkan_device.hpp"
#include "vulkan_pipeline.hpp"
#include "vulkan_swapchain.hpp"
#include "vulkan_window.hpp"
#include <vector>
#include <vulkan/vulkan_core.h>

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>

struct PushConstants {
    glm::mat4 model{1.0f};
    bool indirect = 1;
};

class VulkanRenderer {
  public:
    VulkanRenderer(VulkanWindow* window, VulkanDevice* deviceRef, VulkanDescriptors* descriptorManager);
    ~VulkanRenderer();
    void recordCommandBuffer(uint32_t imageIndex);
    void startFrame();
    bool endFrame();
    void beginRendering();
    void endRendering();
    VkCommandBuffer getCurrentCommandBuffer() { return commandBuffers[getCurrentFrame()]; }
    void bindPipeline();
    VulkanDescriptors* descriptorManager;
    void bindDescriptorSet(uint32_t setNum, VkDescriptorSet set);
    void loadImage(std::string path, VkSampler& sampler, VkImageView& imageView);
    VkExtent2D getSwapChainExtent() { return swapChain->getExtent(); }
    VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }
    VulkanWindow* getWindow() { return vulkanWindow; }
    size_t getCurrentFrame() const { return swapChain->currentFrame(); }

  private:
    void init();
    void createCommandBuffers();
    void createPipeline();
    void recreateSwapChain();

    VulkanDevice* device;
    VulkanPipeline* graphicsPipeline;
    VulkanWindow* vulkanWindow;
    VulkanSwapChain* swapChain;

    VkPipelineLayout pipelineLayout;

    std::vector<VkCommandBuffer> commandBuffers;
};

#endif // VULKAN_RENDERER_H_
