#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_IMPLEMENTATION
#include "memory_manager.hpp"
#include "common.hpp"
#include "device.hpp"
#include "vk_mem_alloc.h"
#include <vulkan/vulkan_core.h>

MemoryManager* MemoryManager::memory_manager;

MemoryManager::MemoryManager() {
    VmaVulkanFunctions vulkan_functions = {};
    vulkan_functions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
    vulkan_functions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocatorCreateInfo = {};
    allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT | VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    allocatorCreateInfo.vulkanApiVersion = Device::API_VERSION;
    allocatorCreateInfo.physicalDevice = Device::device->physical_device();
    allocatorCreateInfo.device = Device::device->vk_device();
    allocatorCreateInfo.instance = Device::device->instance();
    allocatorCreateInfo.pVulkanFunctions = &vulkan_functions;

    vmaCreateAllocator(&allocatorCreateInfo, &_allocator);
    memory_manager = this;
}

MemoryManager::~MemoryManager() {
    for (auto buffer : _buffers) {
        delete buffer.second;
    }
    for (auto image : _images) {
        vmaDestroyImage(_allocator, image.second.vk_image, image.second.allocation);
    }
    vmaDestroyAllocator(_allocator);
}

Buffer* MemoryManager::create_buffer(std::string name, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
    if (_buffers.count(name) != 0) {
        throw std::runtime_error(name + " already exists!");
    }

    Buffer* buffer = new Buffer(_allocator, size, usage, properties);
    Device::device->set_debug_name(VK_OBJECT_TYPE_BUFFER, (uint64_t)buffer->vk_buffer(), name);

    // FIXME:
    // Thread safety
    _buffers.insert({name, buffer});
    return buffer;
}

void MemoryManager::delete_buffer(std::string name) {
    delete _buffers[name];
    _buffers.erase(name);
};

void MemoryManager::create_image(std::string name, uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples,
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

void MemoryManager::delete_image(std::string name) {
    Image image = get_image(name);
    vmaDestroyImage(_allocator, image.vk_image, image.allocation);
    _images.erase(name);
};
