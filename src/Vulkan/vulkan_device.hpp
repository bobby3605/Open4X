#ifndef VULKAN_DEVICE_H_
#define VULKAN_DEVICE_H_

#include "vulkan_window.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>
#include <vulkan/vulkan_core.h>

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() { return graphicsFamily.has_value() && presentFamily.has_value(); }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class VulkanDevice {
  public:
#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

    VulkanDevice(VulkanWindow* window);
    ~VulkanDevice();

    VkCommandPool getCommandPool() { return commandPool_; }
    VkDevice device() { return device_; }
    VkSurfaceKHR surface() { return surface_; }
    VkQueue graphicsQueue() { return graphicsQueue_; }
    VkQueue presentQueue() { return presentQueue_; }
    VkPhysicalDevice getPhysicalDevice() { return physicalDevice; }
    const VkSampleCountFlagBits getMsaaSamples() const { return msaaSamples; }
    const VkBool32 getSampleShading() const { return sampleShading; }

    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
    SwapChainSupportDetails getSwapChainSupport() { return querySwapChainSupport(physicalDevice); }
    QueueFamilyIndices findPhysicalQueueFamilies() { return findQueueFamilies(physicalDevice); }

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer,
                      VkDeviceMemory& bufferMemory);

    void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format,
                     VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image,
                     VkDeviceMemory& imageMemory);

    VkFence getFence();
    void releaseFence(VkFence fence);

    class singleTimeBuilder {
      public:
        singleTimeBuilder(VulkanDevice* vulkanDevice, VkCommandPool commandPool);
        singleTimeBuilder& copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
        singleTimeBuilder& transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout,
                                                 uint32_t mipLevels);
        singleTimeBuilder& copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
        singleTimeBuilder& generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
        void run();

      private:
        VkCommandPool commandPool;
        VkCommandBuffer commandBuffer;
        VulkanDevice* vulkanDevice;

        void beginSingleTimeCommands();
        void endSingleTimeCommands();
    };

  public:
    singleTimeBuilder& singleTimeCommands();

  private:
    VulkanWindow* window;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkCommandPool commandPool_;
    VkCommandPool singleTimeCommandPool;

    VkDevice device_;
    VkSurfaceKHR surface_;
    VkQueue graphicsQueue_;
    VkQueue presentQueue_;

    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    static const VkBool32 msaaEnable = VK_FALSE;
    static const VkBool32 sampleShading = VK_FALSE;
    VkPhysicalDeviceVulkan12Features vk12_features{};
    VkPhysicalDeviceShaderDrawParameterFeatures ext_feature{};
    VkPhysicalDeviceFeatures2 deviceFeatures{};
    bool checkFeatures(VkPhysicalDevice device);

    void createInstance();
    void setupDebugMessenger();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    VkCommandPool createCommandPool(VkCommandPoolCreateFlags flags);

    std::vector<VkFence> fencePool;

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    bool isDeviceSuitable(VkPhysicalDevice device);
    std::vector<const char*> getRequiredExtensions();
    bool checkValidationLayerSupport();
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    VkSampleCountFlagBits getMaxUsableSampleCount();

    const std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};
};

#endif // VULKAN_DEVICE_H_
