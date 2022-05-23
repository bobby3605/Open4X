#ifndef VULKAN_DEVICE_H_
#define VULKAN_DEVICE_H_

#include "vulkan_window.hpp"
#include <optional>
#include <vector>

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

  VulkanDevice(VulkanWindow &window);
  ~VulkanDevice();

  VkCommandPool getCommandPool() { return commandPool; }
  VkDevice device() { return device_; }
  VkSurfaceKHR surface() { return surface_; }
  VkQueue graphicsQueue() { return graphicsQueue_; }
  VkQueue presentQueue() { return presentQueue_; }

  SwapChainSupportDetails getSwapChainSupport() { return querySwapChainSupport(); }
  QueueFamilyIndices findPhysicalQueueFamilies() { return findQueueFamilies(); }

private:
  VulkanWindow &window;
  VkInstance instance;
  VkDebugUtilsMessengerEXT debugMessenger;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkCommandPool commandPool;

  VkDevice device_;
  VkSurfaceKHR surface_;
  VkQueue graphicsQueue_;
  VkQueue presentQueue_;

  void createInstance();
  void setupDebugMessenger();
  void createSurface();
  void pickPhysicalDevice();
  void createLogicalDevice();
  void createCommandPool();

  QueueFamilyIndices findQueueFamilies();
  bool checkDeviceExtensionSupport();
  bool isDeviceSuitable();
  std::vector<const char *> getRequiredExtensions();
  bool checkValidationLayerSupport();
  SwapChainSupportDetails querySwapChainSupport();

  const std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
};

#endif // VULKAN_DEVICE_H_
