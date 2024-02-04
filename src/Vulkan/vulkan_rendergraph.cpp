#include "vulkan_rendergraph.hpp"
#include "Include/BaseTypes.h"
#include "common.hpp"
#include "vulkan_buffer.hpp"
#include "vulkan_descriptors.hpp"
#include "vulkan_pipeline.hpp"
#include "vulkan_window.hpp"
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vulkan/vulkan_core.h>

static VkDescriptorType switchStorageType(glslang::TStorageQualifier storageType) {
    switch (storageType) {
    case glslang::EvqUniform:
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    case glslang::EvqBuffer:
        return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    default:
        throw std::runtime_error("switchStorageType unsupported type: " + std::to_string(storageType));
    }
}

VulkanRenderGraph::VulkanRenderGraph(std::shared_ptr<VulkanDevice> device, VulkanWindow* window, std::shared_ptr<Settings> settings)
    : _device{device}, _window{window}, _settings{settings} {
    swapChain = new VulkanSwapChain(device, _window->getExtent());
    createCommandBuffers();
    descriptorManager = std::make_shared<VulkanDescriptors>(device);
}

bool VulkanRenderGraph::render() {
    startFrame();
    recordRenderOps(getCurrentCommandBuffer());
    return endFrame();
}

void VulkanRenderGraph::createCommandBuffers() {
    commandBuffers.resize(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = _device->getCommandPool();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

    checkResult(vkAllocateCommandBuffers(_device->device(), &allocInfo, commandBuffers.data()), "failed to create command buffers");
}

void VulkanRenderGraph::recreateSwapChain() {
    // pause on minimization
    if (_settings->pauseOnMinimization) {
        int width = 0, height = 0;
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(_window->getGLFWwindow(), &width, &height);
            glfwWaitEvents();
        }
    }

    vkDeviceWaitIdle(_device->device());

    swapChain = new VulkanSwapChain(_device, _window->getExtent(), swapChain);
}

void VulkanRenderGraph::startFrame() {
    VkResult result = swapChain->acquireNextImage();

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        // TODO
        // Do I need to run acquireNextImage after this?
        recreateSwapChain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image");
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;

    vkResetCommandBuffer(getCurrentCommandBuffer(), 0);

    checkResult(vkBeginCommandBuffer(getCurrentCommandBuffer(), &beginInfo), "failed to begin recording command buffer");
}

bool VulkanRenderGraph::endFrame() {
    checkResult(vkEndCommandBuffer(getCurrentCommandBuffer()), "failed to end command buffer");
    VkResult result = swapChain->submitCommandBuffers(&commandBuffers[swapChain->currentFrame()]);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        recreateSwapChain();
        // return true if framebuffer was resized
        return true;
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image");
    }
    return false;
}

void VulkanRenderGraph::bufferWrite(std::string name, void* data) { globalBuffers[name]->write(data); }
