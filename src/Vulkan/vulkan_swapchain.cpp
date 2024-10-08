#include "vulkan_swapchain.hpp"
#include "common.hpp"
#include "vulkan_device.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

VulkanSwapChain::VulkanSwapChain(std::shared_ptr<VulkanDevice> deviceRef, VkExtent2D windowExtent)
    : device{deviceRef}, windowExtent{windowExtent}, oldSwapChain(VK_NULL_HANDLE) {
    init();
}

VulkanSwapChain::VulkanSwapChain(std::shared_ptr<VulkanDevice> deviceRef, VkExtent2D windowExtent, VulkanSwapChain* oldSwapChain)
    : device{deviceRef}, windowExtent{windowExtent}, oldSwapChain(oldSwapChain->swapChain) {
    init();
    delete oldSwapChain;
}

VulkanSwapChain::~VulkanSwapChain() {
    vkDestroyImageView(device->device(), colorImageView, nullptr);
    vkDestroyImage(device->device(), colorImage, nullptr);
    vkFreeMemory(device->device(), colorImageMemory, nullptr);

    vkDestroyImageView(device->device(), depthImageView, nullptr);
    vkDestroyImage(device->device(), depthImage, nullptr);
    vkFreeMemory(device->device(), depthImageMemory, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device->device(), renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device->device(), imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device->device(), inFlightFences[i], nullptr);
    }

    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        vkDestroyImageView(device->device(), swapChainImageViews[i], nullptr);
    }

    vkDestroySwapchainKHR(device->device(), swapChain, nullptr);
}

void VulkanSwapChain::init() {
    createSwapChain();
    createImageViews();
    createColorResources();
    createDepthResources();
    createSyncObjects();
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        // No syncing, least input lag
        if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            return availablePresentMode;
        }

        // Triple Buffering
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    // VSYNC
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanSwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        VkExtent2D actualExtent = windowExtent;
        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

void VulkanSwapChain::createSwapChain() {
    SwapChainSupportDetails swapChainSupport = device->getSwapChainSupport();

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = device->surface();
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.oldSwapchain = oldSwapChain;

    QueueFamilyIndices indices = device->findPhysicalQueueFamilies();
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    checkResult(vkCreateSwapchainKHR(device->device(), &createInfo, nullptr, &swapChain), "failed to create swap chain");

    vkGetSwapchainImagesKHR(device->device(), swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device->device(), swapChain, &imageCount, swapChainImages.data());
    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

void VulkanSwapChain::createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        swapChainImageViews[i] = device->createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        device->setDebugName(VK_OBJECT_TYPE_IMAGE, (uint64_t)swapChainImages[i], "SwapChainImage[" + std::to_string(i) + "]");
    }
}

void VulkanSwapChain::createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        checkResult((vkCreateSemaphore(device->device(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) |
                     vkCreateSemaphore(device->device(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) |
                     vkCreateFence(device->device(), &fenceInfo, nullptr, &inFlightFences[i])),
                    "failed to create synchronization objects for a frame!");
    }
}

VkResult VulkanSwapChain::acquireNextImage() {
    vkWaitForFences(device->device(), 1, &inFlightFences[currentFrame()], VK_TRUE, UINT64_MAX);

    VkResult result = vkAcquireNextImageKHR(device->device(), swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame()],
                                            VK_NULL_HANDLE, &imageIndex);

    vkResetFences(device->device(), 1, &inFlightFences[currentFrame()]);

    return result;
}

VkResult VulkanSwapChain::submitCommandBuffers(const VkCommandBuffer* buffer) {

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame()]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = buffer;

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame()]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    checkResult(vkQueueSubmit(device->graphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame()]),
                "failed to submit draw command buffer");

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    VkResult result = vkQueuePresentKHR(device->presentQueue(), &presentInfo);

    _currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

    return result;
}

void VulkanSwapChain::createDepthResources() {
    VkFormat depthFormat = findDepthFormat();
    device->createImage(swapChainExtent.width, swapChainExtent.height, 1, device->getMsaaSamples(), depthFormat, VK_IMAGE_TILING_OPTIMAL,
                        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
    device->setDebugName(VK_OBJECT_TYPE_IMAGE, (uint64_t)depthImage, "depthImage");
    depthImageView = device->createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
}

void VulkanSwapChain::createColorResources() {
    VkFormat colorFormat = swapChainImageFormat;

    device->createImage(swapChainExtent.width, swapChainExtent.height, 1, device->getMsaaSamples(), colorFormat, VK_IMAGE_TILING_OPTIMAL,
                        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                        colorImage, colorImageMemory);
    device->setDebugName(VK_OBJECT_TYPE_IMAGE, (uint64_t)colorImage, "colorImage");
    colorImageView = device->createImageView(colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

VkFormat VulkanSwapChain::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling,
                                              VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(device->getPhysicalDevice(), format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format");
}

VkFormat VulkanSwapChain::findDepthFormat() {
    return findSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, VK_IMAGE_TILING_OPTIMAL,
                               VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

bool VulkanSwapChain::hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}
