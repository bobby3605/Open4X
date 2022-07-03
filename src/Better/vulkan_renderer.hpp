#ifndef VULKAN_RENDERER_H_
#define VULKAN_RENDERER_H_

#include "Renderer.hpp"
#include "vulkan_descriptors.hpp"
#include "vulkan_device.hpp"
#include "vulkan_pipeline.hpp"
#include "vulkan_swapchain.hpp"
#include "vulkan_window.hpp"

class VulkanRenderer {
public:
  VulkanRenderer(VulkanWindow *window, VulkanDevice *deviceRef);
  ~VulkanRenderer();
  void recordCommandBuffer(uint32_t imageIndex);
  void startFrame();
  void endFrame();
  void beginSwapChainrenderPass();
  void endSwapChainrenderPass();
  VkCommandBuffer getCurrentCommandBuffer();
  uint32_t getCurrentFrame() { return currentFrame; }
  void bindPipeline();
  void bindDescriptorSets();
  VulkanDescriptors *descriptors;
  void createDescriptorSets(std::vector<VkDescriptorBufferInfo> bufferInfos);

private:
  void init();
  void createCommandBuffers();
  void createPipeline();
  void recreateSwapChain();
  void createDescriptors();

  VulkanDevice *device;
  VulkanPipeline *graphicsPipeline;
  VulkanWindow *vulkanWindow;
  VulkanSwapChain *swapChain;

  VkPipelineLayout pipelineLayout;
  VkSampler textureSampler;
  VkImage textureImage;
  VkImageView textureImageView;
  VkDeviceMemory textureImageMemory;

  std::vector<VkCommandBuffer> commandBuffers;
  uint32_t currentFrame = 0;
  uint32_t imageIndex = 0;
};

#endif // VULKAN_RENDERER_H_
