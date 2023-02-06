#ifndef VULKAN_RENDERER_H_
#define VULKAN_RENDERER_H_

#include "vulkan_buffer.hpp"
#include "vulkan_descriptors.hpp"
#include "vulkan_device.hpp"
#include "vulkan_pipeline.hpp"
#include "vulkan_swapchain.hpp"
#include "vulkan_window.hpp"
#include <memory>
#include <string>
#include <unordered_map>
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
    uint32_t totalInstanceCount;
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
    VulkanRenderer(VulkanWindow* window, VulkanDevice* deviceRef, VulkanDescriptors* descriptorManager,
                   const std::vector<VkDrawIndexedIndirectCommand>& drawCommands);
    ~VulkanRenderer();
    void recordCommandBuffer(uint32_t imageIndex);
    void startFrame();
    bool endFrame();
    void beginRendering();
    void endRendering();
    VkCommandBuffer getCurrentCommandBuffer() { return commandBuffers[getCurrentFrame()]; }
    void bindPipeline();
    VulkanDescriptors* descriptorManager;
    void bindDescriptorSet(VkPipelineBindPoint bindPoint, VkPipelineLayout layout, uint32_t setNum, VkDescriptorSet set);
    void loadImage(std::string path, VkSampler& sampler, VkImageView& imageView);
    VkExtent2D getSwapChainExtent() { return swapChain->getExtent(); }
    VulkanWindow* getWindow() { return vulkanWindow; }
    size_t getCurrentFrame() const { return swapChain->currentFrame(); }
    VkPipelineLayout graphicsPipelineLayout() const { return pipelineLayout; }
    void cullDraws(const std::vector<VkDrawIndexedIndirectCommand>& drawCommands, ComputePushConstants& frustumCullPushConstants);

  private:
    void init(const std::vector<VkDrawIndexedIndirectCommand>& drawCommands);
    void createCommandBuffers();
    void createPipeline();
    void createComputePipeline(std::string name, std::vector<VkDescriptorSetLayout>& descriptorLayouts,
                               std::vector<VkPushConstantRange>& pushConstants, VkSpecializationInfo* specializationInfo = VK_NULL_HANDLE);
    void createCullingPipelines(const std::vector<VkDrawIndexedIndirectCommand>& drawCommands);
    void recreateSwapChain();
    void bindComputePipeline(std::string name);
    void memoryBarrier(VkAccessFlags2 srcAccessMask, VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 dstAccessMask,
                       VkPipelineStageFlags2 dstStageMask);

    VulkanDevice* device;
    VulkanPipeline* graphicsPipeline;
    VulkanWindow* vulkanWindow;
    VulkanSwapChain* swapChain;

    VkPipelineLayout pipelineLayout;
    std::unordered_map<std::string, std::shared_ptr<VulkanPipeline>> computePipelines;

    std::vector<VkCommandBuffer> commandBuffers;

    // compute culling buffers
    std::shared_ptr<VulkanBuffer> culledDrawIndirectCount;
    std::shared_ptr<VulkanBuffer> culledDrawCommandsBuffer;

    uint drawCount;

    friend class VulkanObjects;
};

#endif // VULKAN_RENDERER_H_
