#include "swapchain.hpp"
#include "common.hpp"
#include "device.hpp"
#include "memory_manager.hpp"
#include <algorithm>
#include <vulkan/vulkan_core.h>

SwapChain::SwapChain(VkExtent2D extent) : _extent(extent) {
    create_swap_chain();
    create_image_views();
    create_color_resources();
    create_depth_resources();
    create_sync_objects();
}

SwapChain::~SwapChain() {
    vkDestroyImageView(Device::device->vk_device(), _color_image_view, nullptr);
    vkDestroyImageView(Device::device->vk_device(), _depth_image_view, nullptr);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(Device::device->vk_device(), _render_finished_semaphores[i], nullptr);
        vkDestroySemaphore(Device::device->vk_device(), _image_available_semaphores[i], nullptr);
        vkDestroyFence(Device::device->vk_device(), _in_flight_fences[i], nullptr);
    }

    for (size_t i = 0; i < _swap_chain_image_views.size(); i++) {
        vkDestroyImageView(Device::device->vk_device(), _swap_chain_image_views[i], nullptr);
    }

    vkDestroySwapchainKHR(Device::device->vk_device(), _swap_chain, nullptr);
}

void SwapChain::choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats) {
    for (const auto& available_format : available_formats) {
        if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            _surface_format = available_format;
        }
    }

    _surface_format = available_formats[0];
}

VkPresentModeKHR choose_swap_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes) {
    for (const auto& available_present_mode : available_present_modes) {
        // No syncing, least input lag
        if (available_present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            return available_present_mode;
        }

        // Triple Buffering
        if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return available_present_mode;
        }
    }

    // VSYNC
    return VK_PRESENT_MODE_FIFO_KHR;
}

void SwapChain::clamp_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        _extent = capabilities.currentExtent;
    } else {
        _extent.width = std::clamp(_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        _extent.height = std::clamp(_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }
}
void SwapChain::create_swap_chain(VkSwapchainKHR _old_swap_chain) {
    Device::SwapChainSupportDetails swap_chain_support = Device::device->query_swap_chain_support();

    choose_swap_surface_format(swap_chain_support.formats);
    VkPresentModeKHR presentMode = choose_swap_present_mode(swap_chain_support.presentModes);
    clamp_swap_extent(swap_chain_support.capabilities);

    uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;
    if (swap_chain_support.capabilities.maxImageCount > 0 && image_count > swap_chain_support.capabilities.maxImageCount) {
        image_count = swap_chain_support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = Device::device->surface();
    createInfo.minImageCount = image_count;
    createInfo.imageFormat = _surface_format.format;
    createInfo.imageColorSpace = _surface_format.colorSpace;
    createInfo.imageExtent = _extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.oldSwapchain = _old_swap_chain;

    Device::QueueFamilyIndices indices = Device::device->find_queue_families();
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

    createInfo.preTransform = swap_chain_support.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    check_result(vkCreateSwapchainKHR(Device::device->vk_device(), &createInfo, nullptr, &_swap_chain), "failed to create swap chain");

    vkGetSwapchainImagesKHR(Device::device->vk_device(), _swap_chain, &image_count, nullptr);
    _swap_chain_images.resize(image_count);
    vkGetSwapchainImagesKHR(Device::device->vk_device(), _swap_chain, &image_count, _swap_chain_images.data());
    for (size_t i = 0; i < _swap_chain_images.size(); ++i) {
        Device::device->set_debug_name(VK_OBJECT_TYPE_IMAGE, (uint64_t)_swap_chain_images[i],
                                       "swap_chain_image[" + std::to_string(i) + "]");
    }
}

void SwapChain::create_image_views() {
    _swap_chain_image_views.resize(_swap_chain_images.size());
    for (size_t i = 0; i < _swap_chain_images.size(); i++) {
        _swap_chain_image_views[i] =
            Device::device->create_image_view("swap_chain_image_view[" + std::to_string(i) + "]", _swap_chain_images[i],
                                              _surface_format.format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }
}

void SwapChain::create_color_resources() {

    _color_image =
        MemoryManager::memory_manager
            ->create_image("color_image", _extent.width, _extent.height, 1, Device::device->msaa_samples(), _surface_format.format,
                           VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
            .vk_image;
    _color_image_view =
        Device::device->create_image_view("color_image_view", MemoryManager::memory_manager->get_image("color_image").vk_image,
                                          _surface_format.format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

VkFormat SwapChain::find_supported_format(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(Device::device->physical_device(), format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format");
}

VkFormat SwapChain::find_depth_format() {
    return find_supported_format({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, VK_IMAGE_TILING_OPTIMAL,
                                 VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void SwapChain::create_depth_resources() {

    _depth_format = find_depth_format();
    _depth_image =
        MemoryManager::memory_manager
            ->create_image("depth_image", _extent.width, _extent.height, 1, Device::device->msaa_samples(), _depth_format,
                           VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
            .vk_image;
    _depth_image_view = Device::device->create_image_view(
        "depth_image_view", MemoryManager::memory_manager->get_image("depth_image").vk_image, _depth_format, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
}

void SwapChain::create_sync_objects() {

    _image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    _render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    _in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphore_info{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

    VkFenceCreateInfo fence_info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        check_result((vkCreateSemaphore(Device::device->vk_device(), &semaphore_info, nullptr, &_image_available_semaphores[i]) |
                      vkCreateSemaphore(Device::device->vk_device(), &semaphore_info, nullptr, &_render_finished_semaphores[i]) |
                      vkCreateFence(Device::device->vk_device(), &fence_info, nullptr, &_in_flight_fences[i])),
                     "failed to create synchronization objects for a frame!");
    }
}

VkResult SwapChain::acquire_next_image() {
    vkWaitForFences(Device::device->vk_device(), 1, &_in_flight_fences[current_frame()], VK_TRUE, UINT64_MAX);

    VkResult result = vkAcquireNextImageKHR(Device::device->vk_device(), _swap_chain, UINT64_MAX,
                                            _image_available_semaphores[current_frame()], VK_NULL_HANDLE, &_image_index);

    vkResetFences(Device::device->vk_device(), 1, &_in_flight_fences[current_frame()]);

    return result;
}

VkResult SwapChain::submit_command_buffers(std::vector<VkCommandBuffer>& cmd_buffers) {

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {_image_available_semaphores[current_frame()]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = cmd_buffers.size();
    submitInfo.pCommandBuffers = cmd_buffers.data();

    VkSemaphore signalSemaphores[] = {_render_finished_semaphores[current_frame()]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    check_result(vkQueueSubmit(Device::device->graphics_queue(), 1, &submitInfo, _in_flight_fences[current_frame()]),
                 "failed to submit draw command buffer");

    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swap_chains[] = {_swap_chain};
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swap_chains;
    present_info.pImageIndices = &_image_index;
    present_info.pResults = nullptr;

    VkResult result = vkQueuePresentKHR(Device::device->present_queue(), &present_info);

    _current_frame = (_current_frame + 1) % MAX_FRAMES_IN_FLIGHT;

    return result;
}
