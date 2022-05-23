#include "Open4XVulkan.hpp"
#include "Vertex.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <glm/glm.hpp>
#include <iostream>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

void Open4XVulkan::initVulkan() {
  createInstance();
  setupDebugMessenger();
  createSurface();
  pickPhysicalDevice();
  createLogicalDevice();
  createSwapChain();
  createImageViews();
  createRenderPass();
  createDescriptorSetLayout();
  createGraphicsPipeline();
  createFramebuffers();
  createTextureImage();
  createTextureImageView();
  createTextureSampler();
  createVertexBuffer();
  createIndexBuffer();
  createUniformBuffers();
  createDescriptorPool();
  createDescriptorSets();
  createCommandBuffers();
  createSyncObjects();
}

Open4XVulkan::~Open4XVulkan() {
  vkDeviceWaitIdle(device);

  cleanupSwapChain();

  vkDestroySampler(device, textureSampler, nullptr);
  vkDestroyImageView(device, textureImageView, nullptr);

  vkDestroyImage(device, textureImage, nullptr);
  vkFreeMemory(device, textureImageMemory, nullptr);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroyBuffer(device, uniformBuffers[i], nullptr);
    vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
  }

  vkDestroyDescriptorPool(device, descriptorPool, nullptr);

  vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

  vkDestroyBuffer(device, indexBuffer, nullptr);
  vkFreeMemory(device, indexBufferMemory, nullptr);

  vkDestroyBuffer(device, vertexBuffer, nullptr);
  vkFreeMemory(device, vertexBufferMemory, nullptr);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
    vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
    vkDestroyFence(device, inFlightFences[i], nullptr);
  }

  vkDestroyCommandPool(device, commandPool, nullptr);

  vkDestroyDevice(device, nullptr);

  if (enableValidationLayers) {
    DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
  }

  vkDestroySurfaceKHR(instance, surface, nullptr);
  vkDestroyInstance(instance, nullptr);

  glfwDestroyWindow(window);
  glfwTerminate();
}

/*
** run
*/
Open4XVulkan::Open4XVulkan() {
  initWindow();
  initVulkan();
}
