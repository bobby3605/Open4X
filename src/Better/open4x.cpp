#include "open4x.hpp"
#include <chrono>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cstring>
#include "vulkan_buffer.hpp"
#include "vulkan_swapchain.hpp"

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, 1);
  }
}

Open4X::Open4X() {

  vulkanWindow = new VulkanWindow(640, 480, "Open 4X");
  vulkanDevice = new VulkanDevice(*vulkanWindow);
  vulkanRenderer = new VulkanRenderer(*vulkanWindow, *vulkanDevice);

  glfwSetKeyCallback(vulkanWindow->getGLFWwindow(), key_callback);

  run();

}

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

  std::vector<VulkanBuffer*> uniformBuffers(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);

  for (auto uniformBuffer : uniformBuffers) {
    uniformBuffer = new VulkanBuffer(*vulkanDevice, sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    uniformBuffer->map();
  }

  while (!glfwWindowShouldClose(vulkanWindow->getGLFWwindow())) {
    glfwPollEvents();

    vulkanRenderer->startFrame();

    // TODO
    // move uniform buffers into its own object
 static auto startTime = std::chrono::high_resolution_clock::now();

  auto currentTime = std::chrono::high_resolution_clock::now();
  float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

  UniformBufferObject ubo{};
  ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  ubo.proj = glm::perspective(glm::radians(45.0f), vulkanWindow->getExtent().width / (float)vulkanWindow->getExtent().height, 0.1f, 10.0f);
  ubo.proj[1][1] *= -1;

  uniformBuffers[vulkanRenderer->getCurrentFrame()]->write(static_cast<void*>(&ubo), sizeof(ubo), 0);

    vulkanRenderer->beginSwapChainrenderPass();
/*
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                          &descriptorSets[currentFrame], 0, nullptr);

  VkBuffer vertexBuffers[] = {vertexBuffer};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

  vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

  vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
 */


    vulkanRenderer->endSwapChainrenderPass();

    vulkanRenderer->endFrame();



  }
}
