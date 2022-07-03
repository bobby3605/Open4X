#include "open4x.hpp"
#include "common.hpp"
#include "vulkan_buffer.hpp"
#include "vulkan_model.hpp"
#include "vulkan_swapchain.hpp"
#include <GLFW/glfw3.h>
#include <chrono>
#include <cstring>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, 1);
  }
}

Open4X::Open4X() {}

Open4X::~Open4X() {
  delete vulkanRenderer;
  delete vulkanDevice;
  delete vulkanWindow;
}

struct UniformBufferObject {
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 proj;
};

void Open4X::run() {

  vulkanWindow = new VulkanWindow(640, 480, "Open 4X");
  glfwSetKeyCallback(vulkanWindow->getGLFWwindow(), key_callback);

  vulkanDevice = new VulkanDevice(vulkanWindow);
  vulkanRenderer = new VulkanRenderer(vulkanWindow, vulkanDevice);

  VkDeviceSize vertexBufferSize = sizeof(VulkanModel::vertices[0]) * VulkanModel::vertices.size();

  VulkanBuffer vertexStagingBuffer(*vulkanDevice, vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  vertexStagingBuffer.map();
  vertexStagingBuffer.write((void *)VulkanModel::vertices.data(), vertexBufferSize, 0);
  vertexStagingBuffer.unmap();

  VulkanBuffer vertexBuffer(*vulkanDevice, vertexBufferSize,
                            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  vulkanDevice->copyBuffer(vertexStagingBuffer.buffer, vertexBuffer.buffer, vertexBufferSize);

  std::vector<VulkanBuffer *> uniformBuffers(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);

  for (size_t i = 0; i < VulkanSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
    uniformBuffers[i] = new VulkanBuffer(*vulkanDevice, sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    uniformBuffers[i]->map();
  }

  std::vector<VkDescriptorBufferInfo> bufferInfos;
  bufferInfos.resize(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);

  for (uint32_t i = 0; i < VulkanSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
    bufferInfos[i].buffer = uniformBuffers[i]->buffer;
    bufferInfos[i].offset = 0;
    bufferInfos[i].range = sizeof(UniformBufferObject);
  }

  vulkanRenderer->createDescriptorSets(bufferInfos);

  VkDeviceSize indexBufferSize = sizeof(VulkanModel::indices[0]) * VulkanModel::indices.size();

  VulkanBuffer indexStagingBuffer(*vulkanDevice, indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  indexStagingBuffer.map();

  indexStagingBuffer.write((void *)VulkanModel::indices.data(), (size_t)indexBufferSize, 0);
  indexStagingBuffer.unmap();

  VulkanBuffer indexBuffer(*vulkanDevice, indexBufferSize,
                           VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  vulkanDevice->copyBuffer(indexStagingBuffer.buffer, indexBuffer.buffer, indexBufferSize);

  while (!glfwWindowShouldClose(vulkanWindow->getGLFWwindow())) {
    glfwPollEvents();

    vulkanRenderer->startFrame();

    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(
        glm::radians(45.0f),
        vulkanRenderer->getSwapChainExtent().width / (float)vulkanRenderer->getSwapChainExtent().height, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1;

    uniformBuffers[vulkanRenderer->getCurrentFrame()]->write(&ubo, sizeof(ubo), 0);

    vulkanRenderer->beginSwapChainrenderPass();
    vulkanRenderer->bindPipeline();

    VkBuffer vertexBuffers[] = {vertexBuffer.buffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(vulkanRenderer->getCurrentCommandBuffer(), 0, 1, vertexBuffers, offsets);

    vkCmdBindIndexBuffer(vulkanRenderer->getCurrentCommandBuffer(), indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);

    vulkanRenderer->bindDescriptorSets();

    vkCmdDrawIndexed(vulkanRenderer->getCurrentCommandBuffer(), static_cast<uint32_t>(VulkanModel::indices.size()), 1,
                     0, 0, 0);

    vulkanRenderer->endSwapChainrenderPass();

    vulkanRenderer->endFrame();
  }
  vkDeviceWaitIdle(vulkanDevice->device());
  for (size_t i = 0; i < VulkanSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
    delete uniformBuffers[i];
  }
}
