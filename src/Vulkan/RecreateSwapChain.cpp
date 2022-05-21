#include "Open4XVulkan.hpp"

// TODO
// https://vulkan-tutorial.com/en/Drawing_a_triangle/Swap_chain_recreation
// However, the disadvantage of this approach is that we need to stop all rendering before creating the new swap chain.
// It is possible to create a new swap chain while drawing commands on an image from the old swap chain are still
// in-flight. You need to pass the previous swap chain to the oldSwapChain field in the VkSwapchainCreateInfoKHR struct
// and destroy the old swap chain as soon as you've finished using it.

void Open4XVulkan::recreateSwapChain() {
  // Pause when minimized
  int width = 0, height = 0;
  glfwGetFramebufferSize(window, &width, &height);
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(window, &width, &height);
    glfwWaitEvents();
  }

  vkDeviceWaitIdle(device);

  cleanupSwapChain();

  createSwapChain();
  createImageViews();
  createRenderPass();
  // graphics pipeline doesn't need to be recreated if viewports and scissor rectangles are dynamic
  // commenting this out causes a crash currently
  createGraphicsPipeline();
  createFramebuffers();
}
