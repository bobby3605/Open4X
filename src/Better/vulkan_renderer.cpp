#include "vulkan_renderer.hpp"
#include "common.hpp"
#include "vulkan_swapchain.hpp"
#include <stdexcept>

VulkanRenderer::VulkanRenderer(VulkanDevice &deviceRef, VulkanSwapChain &swapChainRef)
    : device{deviceRef}, swapChain(swapChainRef) {
  init();
}

void VulkanRenderer::init() {
  createSwapChain();
  createCommandBuffers();
  createPipeline();
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

void VulkanRenderer::beginSwapChainrenderPass(VkCommandBuffer commandBuffer) {
  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = swapChain.getRenderPass();
  renderPassInfo.framebuffer = swapChain.getFramebuffer(imageIndex);
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = swapChain.getExtent();
  VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clearColor;

  vkCmdBeginRenderPass(commandBuffers[currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanRenderer::recordCommandBuffer(uint32_t imageIndex) {
  vkResetCommandBuffer(commandBuffers[currentFrame], 0);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = 0;
  beginInfo.pInheritanceInfo = nullptr;

  checkResult(vkBeginCommandBuffer(commandBuffers[currentFrame], &beginInfo),
              "failed to begin recording command buffer");

  vkCmdBindPipeline(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.getPipeline());

  VkBuffer vertexBuffers[] = {vertexBuffer};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

  vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                          &descriptorSets[currentFrame], 0, nullptr);
  vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
  vkCmdEndRenderPass(commandBuffer);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer");
  }
}

void VulkanRenderer::drawFrame() {
  VkResult result = swapChain.acquireNextImage();

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapChain();
    return;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("failed to acquire swap chain image");
  }

  // updates objects, must run before vkQueueSubmit
  updateUniformBuffer(currentFrame);

  // Start of End frame
  // Move to swapChain, actually draws to the swapChain
  if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
    throw std::runtime_error("failed to submit draw command buffer");
  }
  //

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;

  VkSwapchainKHR swapChains[] = {swapChain};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = &imageIndex;
  presentInfo.pResults = nullptr;

  result = vkQueuePresentKHR(presentQueue, &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
    framebufferResized = false;
    recreateSwapChain();
    return;
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swap chain image");
  }

  currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}
