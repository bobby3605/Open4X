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

struct ComputePushConstants {
    uint32_t drawIndirectCount;
    float nearD;
    float farD;
    float ratio;
    float sphereFactorX;
    float sphereFactorY;
    float tang;
    uint32_t pad0;
    glm::vec3 X;
    uint32_t pad1;
    glm::vec3 Y;
    uint32_t pad2;
    glm::vec3 Z;
    uint32_t pad3;
    glm::vec3 camPos;
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
    void bindComputePipeline();
    void runComputePipeline(VkDescriptorSet computeSet, VkBuffer indirectCountBuffer, ComputePushConstants& computePushConstants);
    VulkanDescriptors* descriptorManager;
    void bindDescriptorSet(VkPipelineBindPoint bindPoint, VkPipelineLayout layout, uint32_t setNum, VkDescriptorSet set);
    void loadImage(std::string path, VkSampler& sampler, VkImageView& imageView);
    VkExtent2D getSwapChainExtent() { return swapChain->getExtent(); }
    VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }
    VulkanWindow* getWindow() { return vulkanWindow; }
    size_t getCurrentFrame() const { return swapChain->currentFrame(); }
    VkPipelineLayout graphicsPipelineLayout() const { return pipelineLayout; }

  private:
    void init();
    void createCommandBuffers();
    void createPipeline();
    void createComputePipeline();
    void recreateSwapChain();

    VulkanDevice* device;
    VulkanPipeline* graphicsPipeline;
    VulkanPipeline* computePipeline;
    VulkanWindow* vulkanWindow;
    VulkanSwapChain* swapChain;

    VkPipelineLayout pipelineLayout;
    VkPipelineLayout computePipelineLayout;

    std::vector<VkCommandBuffer> commandBuffers;
};

#endif // VULKAN_RENDERER_H_
