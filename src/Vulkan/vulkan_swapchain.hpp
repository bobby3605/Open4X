#ifndef VULKAN_SWAPCHAIN_H_
#define VULKAN_SWAPCHAIN_H_

#include "vulkan_device.hpp"
#include <vulkan/vulkan.h>

class VulkanSwapChain {
public:
  const static int MAX_FRAMES_IN_FLIGHT = 2;

  VulkanSwapChain(VulkanDevice *deviceRef, VkExtent2D windowExtent);
  ~VulkanSwapChain();
  VkResult acquireNextImage(uint32_t *imageIndex);
  VkResult submitCommandBuffers(const VkCommandBuffer *buffer, uint32_t *imageIndex);

  VkRenderPass getRenderPass() { return renderPass; }
  VkFramebuffer getFramebuffer(uint32_t index) { return swapChainFramebuffers[index]; }
  VkExtent2D getExtent() { return swapChainExtent; }
  VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

private:
  void init();
  void createSwapChain();
  void createImageViews();
  void createRenderPass();
  void createDepthResources();
  void createFramebuffers();
  void createSyncObjects();
  void startFrame();

  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
  VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling,
                               VkFormatFeatureFlags features);
  VkFormat findDepthFormat();
  bool hasStencilComponent(VkFormat format);

  VulkanDevice *device;
  VkSwapchainKHR swapChain;
  VkRenderPass renderPass;

  VkFormat swapChainImageFormat;

  VkExtent2D windowExtent;
  VkExtent2D swapChainExtent;

  size_t currentFrame = 0;

  std::vector<VkImage> swapChainImages;
  std::vector<VkImageView> swapChainImageViews;
  std::vector<VkFramebuffer> swapChainFramebuffers;
  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkFence> inFlightFences;

  VkImage depthImage;
  VkDeviceMemory depthImageMemory;
  VkImageView depthImageView;
};

#endif // VULKAN_SWAPCHAIN_H_
