#include "common.hpp"
#include <cstdint>
#include <vulkan/vulkan_core.h>
#define VMA_IMPLEMENTATION
#include "buffers.hpp"
#include "device.hpp"

Buffers* Buffers::buffers;

Buffers::Buffers() {
    VmaVulkanFunctions vulkan_functions = {};
    vulkan_functions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
    vulkan_functions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocatorCreateInfo = {};
    allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
    allocatorCreateInfo.vulkanApiVersion = Device::API_VERSION;
    allocatorCreateInfo.physicalDevice = Device::device->physical_device();
    allocatorCreateInfo.device = Device::device->vk_device();
    allocatorCreateInfo.instance = Device::device->instance();
    allocatorCreateInfo.pVulkanFunctions = &vulkan_functions;

    vmaCreateAllocator(&allocatorCreateInfo, &_allocator);
    buffers = this;
}

Buffers::~Buffers() {
    for (auto buffer : _buffers) {
        vmaDestroyBuffer(_allocator, buffer.second.vk_buffer, buffer.second.allocation);
    }
    for (auto image : _images) {
        vmaDestroyImage(_allocator, image.second.vk_image, image.second.allocation);
    }
    vmaDestroyAllocator(_allocator);
}

void Buffers::create_buffer(std::string name, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
    if (_buffers.count(name) != 0) {
        throw std::runtime_error(name + " already exists!");
    }

    VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.size = size;
    buffer_info.usage = usage;

    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.requiredFlags = properties;
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

    Buffer buffer;
    check_result(vmaCreateBuffer(_allocator, &buffer_info, &alloc_info, &buffer.vk_buffer, &buffer.allocation, nullptr),
                 "failed to create buffer!");
    Device::device->set_debug_name(VK_OBJECT_TYPE_BUFFER, (uint64_t)buffer.vk_buffer, name);
    _buffers.insert({name, buffer});
}

void Buffers::delete_buffer(std::string name) {
    Buffer buffer = get_buffer(name);
    vmaDestroyBuffer(_allocator, buffer.vk_buffer, buffer.allocation);
    _buffers.erase(name);
};

void Buffers::create_image(std::string name, uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples,
                           VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties) {
    if (_images.count(name) != 0) {
        throw std::runtime_error(name + " already exists!");
    }

    VkImageCreateInfo image_info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = width;
    image_info.extent.height = height;
    image_info.extent.depth = 1;
    image_info.mipLevels = mipLevels;
    image_info.arrayLayers = 1;
    image_info.format = format;
    image_info.tiling = tiling;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = usage;
    image_info.samples = numSamples;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.requiredFlags = properties;
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

    Image image;
    check_result(vmaCreateImage(_allocator, &image_info, &alloc_info, &image.vk_image, &image.allocation, nullptr),
                 "failed to create image!");
    Device::device->set_debug_name(VK_OBJECT_TYPE_IMAGE, (uint64_t)image.vk_image, name);
    _images.insert({name, image});
}

void Buffers::delete_image(std::string name) {
    Image image = get_image(name);
    vmaDestroyImage(_allocator, image.vk_image, image.allocation);
    _images.erase(name);
};
