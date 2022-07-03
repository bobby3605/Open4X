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

Open4X::Open4X() {
  vulkanWindow = new VulkanWindow(640, 480, "Open 4X");
  glfwSetKeyCallback(vulkanWindow->getGLFWwindow(), key_callback);

  vulkanDevice = new VulkanDevice(vulkanWindow);
  vulkanRenderer = new VulkanRenderer(vulkanWindow, vulkanDevice);
}

Open4X::~Open4X() {
  delete vulkanRenderer;
  delete vulkanDevice;
  delete vulkanWindow;
}

void Open4X::run() {

  StagedBuffer vertexBuffer(vulkanDevice, (void *)VulkanModel::vertices.data(),
                            sizeof(VulkanModel::vertices[0]) * VulkanModel::vertices.size(),
                            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

  StagedBuffer indexBuffer(vulkanDevice, (void *)VulkanModel::indices.data(),
                           sizeof(VulkanModel::indices[0]) * VulkanModel::indices.size(),
                           VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

  std::vector<UniformBuffer *> uniformBuffers(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);
  std::vector<VkDescriptorBufferInfo> bufferInfos(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);

  for (int i = 0; i < VulkanSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
    uniformBuffers[i] = new UniformBuffer(vulkanDevice);
    bufferInfos[i] = uniformBuffers[i]->getBufferInfo();
  }

  vulkanRenderer->createDescriptorSets(bufferInfos);

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

    uniformBuffers[vulkanRenderer->getCurrentFrame()]->write(&ubo);

    vulkanRenderer->beginSwapChainrenderPass();
    vulkanRenderer->bindPipeline();

    VkBuffer vertexBuffers[] = {vertexBuffer.getBuffer()};
    VkDeviceSize offsets[] = {0};

    vkCmdBindVertexBuffers(vulkanRenderer->getCurrentCommandBuffer(), 0, 1, vertexBuffers, offsets);

    vkCmdBindIndexBuffer(vulkanRenderer->getCurrentCommandBuffer(), indexBuffer.getBuffer(), 0, VK_INDEX_TYPE_UINT16);

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
