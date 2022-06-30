#include "vulkan_renderer.hpp"
#include "common.hpp"
#include "vulkan_swapchain.hpp"
#include "vulkan_window.hpp"
#include <stdexcept>
#include <vulkan/vulkan_core.h>

VulkanRenderer::VulkanRenderer(VulkanWindow &window, VulkanDevice &deviceRef)
  : vulkanWindow{window}, device{deviceRef} {
  init();
}

void VulkanRenderer::init() {
  recreateSwapChain();
  createCommandBuffers();
  createPipeline();
}

VkCommandBuffer VulkanRenderer::getCurrentCommandBuffer() {
  return commandBuffers[currentFrame];
};

void VulkanRenderer::recreateSwapChain() {
  // delete &swapChain;
  swapChain = new VulkanSwapChain(device, vulkanWindow.getExtent());
}

void VulkanRenderer::createPipeline() {}

void VulkanRenderer::createCommandBuffers() {
  commandBuffers.resize(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = device.getCommandPool();
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

  checkResult(vkAllocateCommandBuffers(device.device(), &allocInfo, commandBuffers.data()),
              "failed to create command buffers");
}

void VulkanRenderer::beginSwapChainrenderPass() {
  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = swapChain->getRenderPass();
  renderPassInfo.framebuffer = swapChain->getFramebuffer(imageIndex);
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = swapChain->getExtent();
  VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clearColor;

  vkCmdBeginRenderPass(commandBuffers[currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanRenderer::endSwapChainrenderPass() {
  vkCmdEndRenderPass(commandBuffers[currentFrame]);
}

void VulkanRenderer::startFrame() {
  VkResult result = swapChain->acquireNextImage(imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapChain();
    return;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("failed to acquire swap chain image");
  }

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = 0;
  beginInfo.pInheritanceInfo = nullptr;

  checkResult(vkBeginCommandBuffer(commandBuffers[currentFrame], &beginInfo), "failed to begin recording command buffer");

}

void VulkanRenderer::endFrame() {
  checkResult(vkEndCommandBuffer(commandBuffers[currentFrame]), "failed to end command buffer");
  VkResult result = swapChain->submitCommandBuffers(&commandBuffers[currentFrame], imageIndex);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || vulkanWindow.wasWindowResized()) {
    recreateSwapChain();
    return;
  } else if ( result != VK_SUCCESS ) {
    throw std::runtime_error("failed to present swap chain image");
  }
}
