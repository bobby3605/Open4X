#ifndef OPEN4XVULKAN_H_
#define OPEN4XVULKAN_H_

#include <cstdint>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <optional>
#include <vector>
#include <vulkan/vulkan.h>

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

class Open4XVulkan {
public:
  Open4XVulkan();
  ~Open4XVulkan();
  GLFWwindow *window;
  void drawFrame();
  bool framebufferResized = false;

private:
  void initWindow();
  void initVulkan();
  void pickPhysicalDevice();
  void createLogicalDevice();
  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice);
  bool isDeviceSuitable(VkPhysicalDevice);
  void setupDebugMessenger();
  SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice);
  void createSurface();
  void createInstance();
  void createSwapChain();
  void createImageViews();
  VkShaderModule createShaderModule(const std::vector<char>&);
  void createRenderPass();
  void createGraphicsPipeline();
  void createFramebuffers();
  void createCommandPool();
  void createCommandBuffers();
  void recordCommandBuffer(VkCommandBuffer, uint32_t);
  void createSyncObjects();
  void cleanupSwapChain();
  void recreateSwapChain();


  VkInstance instance;
  VkDebugUtilsMessengerEXT debugMessenger;
  VkSurfaceKHR surface;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkDevice device;
  VkQueue graphicsQueue;
  VkQueue presentQueue;
  VkSwapchainKHR swapChain;
  std::vector<VkImage> swapChainImages;
  VkFormat swapChainImageFormat;
  VkExtent2D swapChainExtent;
  std::vector<VkImageView> swapChainImageViews;
  VkRenderPass renderPass;
  VkPipelineLayout pipelineLayout;
  VkPipeline graphicsPipeline;
  std::vector<VkFramebuffer> swapChainFramebuffers;
  VkCommandPool commandPool;
  std::vector<VkCommandBuffer> commandBuffers;
  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkFence> inFlightFences;
  uint32_t currentFrame = 0;

};

#endif // OPEN4XVULKAN_H_
