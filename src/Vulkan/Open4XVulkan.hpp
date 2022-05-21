#ifndef OPEN4XVULKAN_H_
#define OPEN4XVULKAN_H_
#include "../Renderer.hpp"

#include <cstdint>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <iostream>
#include <optional>
#include <vector>
#include <vulkan/vulkan.h>

// TODO
//
// https://vulkan-tutorial.com/Vertex_buffers/Staging_buffer
// It should be noted that in a real world application, you're not supposed to actually call vkAllocateMemory for every
// individual buffer. The maximum number of simultaneous memory allocations is limited by the maxMemoryAllocationCount
// physical device limit, which may be as low as 4096 even on high end hardware like an NVIDIA GTX 1080. The right way
// to allocate memory for a large number of objects at the same time is to create a custom allocator that splits up a
// single allocation among many different objects by using the offset parameters that we've seen in many functions.
// You can either implement such an allocator yourself, or use the VulkanMemoryAllocator library provided by the GPUOpen
// initiative. However, for this tutorial it's okay to use a separate allocation for every resource, because we won't
// come close to hitting any of these limits for now.
//
// https://vulkan-tutorial.com/en/Vertex_buffers/Index_buffer
// The previous chapter already mentioned that you should allocate multiple resources like buffers from a single memory
// allocation, but in fact you should go a step further. Driver developers recommend that you also store multiple
// buffers, like the vertex and index buffer, into a single VkBuffer and use offsets in commands like
// vkCmdBindVertexBuffers. The advantage is that your data is more cache friendly in that case, because it's closer
// together. It is even possible to reuse the same chunk of memory for multiple resources if they are not used during
// the same render operations, provided that their data is refreshed, of course. This is known as aliasing and some
// Vulkan functions have explicit flags to specify that you want to do this.

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
const int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif
const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};

struct UniformBufferObject {
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 proj;
};

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

class Open4XVulkan : public Renderer {
public:
  Open4XVulkan();
  ~Open4XVulkan();
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
  VkShaderModule createShaderModule(const std::vector<char> &);
  void createRenderPass();
  void createGraphicsPipeline();
  void createFramebuffers();
  void createCommandPool();
  void createVertexBuffer();
  void createCommandBuffers();
  void recordCommandBuffer(VkCommandBuffer, uint32_t);
  void createSyncObjects();
  void cleanupSwapChain();
  void recreateSwapChain();
  uint32_t findMemoryType(uint32_t, VkMemoryPropertyFlags);
  void DestroyDebugUtilsMessengerEXT(VkInstance, VkDebugUtilsMessengerEXT, const VkAllocationCallbacks *);
  void createBuffer(VkDeviceSize, VkBufferUsageFlags, VkMemoryPropertyFlags, VkBuffer &, VkDeviceMemory &);
  void copyBuffer(VkBuffer, VkBuffer, VkDeviceSize);
  void createIndexBuffer();
  void createDescriptorSetLayout();
  void createUniformBuffers();
  void updateUniformBuffer(uint32_t);
  void createDescriptorPool();
  void createDescriptorSets();

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
  VkDescriptorSetLayout descriptorSetLayout;
  VkPipelineLayout pipelineLayout;
  VkPipeline graphicsPipeline;
  std::vector<VkFramebuffer> swapChainFramebuffers;
  VkCommandPool commandPool;
  std::vector<VkCommandBuffer> commandBuffers;
  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkFence> inFlightFences;
  uint32_t currentFrame = 0;
  VkBuffer vertexBuffer;
  VkDeviceMemory vertexBufferMemory;
  VkBuffer indexBuffer;
  VkDeviceMemory indexBufferMemory;
  std::vector<VkBuffer> uniformBuffers;
  std::vector<VkDeviceMemory> uniformBuffersMemory;
  VkDescriptorPool descriptorPool;
  std::vector<VkDescriptorSet> descriptorSets;
};

#endif // OPEN4XVULKAN_H_
