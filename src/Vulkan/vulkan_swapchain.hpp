#ifndef VULKAN_SWAPCHAIN_H_
#define VULKAN_SWAPCHAIN_H_

#include "vulkan_device.hpp"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

class VulkanSwapChain {
  public:
    const static int MAX_FRAMES_IN_FLIGHT = 2;

    VulkanSwapChain(VulkanDevice* deviceRef, VkExtent2D windowExtent);
    VulkanSwapChain(VulkanDevice* deviceRef, VkExtent2D windowExtent, VulkanSwapChain* oldSwapChain);
    ~VulkanSwapChain();
    VkResult acquireNextImage(uint32_t* imageIndex);
    VkResult submitCommandBuffers(const VkCommandBuffer* buffer, uint32_t* imageIndex);

    VkExtent2D getExtent() { return swapChainExtent; }
    VkImage getColorImage() { return colorImage; }
    VkImageView getColorImageView() { return colorImageView; }
    VkImage getDepthImage() { return depthImage; }
    VkImageView getDepthImageView() { return depthImageView; }
    VkImage getSwapChainImage() { return swapChainImages[currentFrame]; }
    VkImageView getSwapChainImageView() { return swapChainImageViews[currentFrame]; }
    VkFormat getSwapChainImageFormat() { return swapChainImageFormat; }
    VkFormat findDepthFormat();

  private:
    void init();
    void createSwapChain();
    void createImageViews();
    void createColorResources();
    void createDepthResources();
    void createSyncObjects();
    void startFrame();

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    bool hasStencilComponent(VkFormat format);

    VulkanDevice* device;
    VkSwapchainKHR swapChain;

    VkFormat swapChainImageFormat;

    VkExtent2D windowExtent;
    VkExtent2D swapChainExtent;

    size_t currentFrame = 0;

    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    VkImage colorImage;
    VkDeviceMemory colorImageMemory;
    VkImageView colorImageView;

    VkSwapchainKHR oldSwapChain;
};

#endif // VULKAN_SWAPCHAIN_H_
