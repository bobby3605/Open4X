#ifndef VULKAN_SWAPCHAIN_H_
#define VULKAN_SWAPCHAIN_H_

#include "vulkan_device.hpp"
#include <vulkan/vulkan.h>

class VulkanSwapChain {
public:
  const static int MAX_FRAMES_IN_FLIGHT = 2;

  VulkanSwapChain(VulkanDevice &deviceRef, VkExtent2D windowExtent);
  ~VulkanSwapChain();
  VkResult acquireNextImage();
  VkResult submitCommandBuffers(const VkCommandBuffer *buffer);

  VkRenderPass getRenderPass() { return renderPass; }
  VkFramebuffer getFramebuffer() { return swapChainFramebuffers[imageIndex]; }
  VkExtent2D getExtent() { return swapChainExtent; }
  VkImageView createImageView(VkImage image, VkFormat format);

private:
  void init();
  void createSwapChain();
  void createImageViews();
  void createRenderPass();
  void createFramebuffers();
  void createSyncObjects();
  void startFrame();

  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

  VulkanDevice &device;
  VkSwapchainKHR swapChain;
  VkRenderPass renderPass;

  VkFormat swapChainImageFormat;

  VkExtent2D windowExtent;
  VkExtent2D swapChainExtent;

  size_t currentFrame = 0;
  uint32_t imageIndex;

  std::vector<VkImage> swapChainImages;
  std::vector<VkImageView> swapChainImageViews;
  std::vector<VkFramebuffer> swapChainFramebuffers;
  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkFence> inFlightFences;
};

#endif // VULKAN_SWAPCHAIN_H_