#ifndef VULKAN_RENDERER_H_
#define VULKAN_RENDERER_H_

#include "Renderer.hpp"
#include "vulkan_device.hpp"
#include "vulkan_pipeline.hpp"
#include "vulkan_swapchain.hpp"

class VulkanRenderer : public Renderer {
public:
  VulkanRenderer(VulkanDevice &deviceRef, VulkanSwapChain &swapChainRef);
  void drawFrame();
  void recordCommandBuffer(uint32_t imageIndex);

private:
  void init();
  void createCommandBuffers();
  void createPipeline();

  void beginSwapChainrenderPass(VkCommandBuffer commandBuffer);

  VulkanDevice &device;
  VulkanSwapChain &swapChain;
  VulkanPipeline *graphicsPipeline;

  std::vector<VkCommandBuffer> commandBuffers;
  uint32_t currentFrame = 0;
  uint32_t imageIndex = 0;
};

#endif // VULKAN_RENDERER_H_
