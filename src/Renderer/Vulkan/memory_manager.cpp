#include <string>
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_IMPLEMENTATION
#include "common.hpp"
#include "device.hpp"
#include "memory_manager.hpp"
#include "vk_mem_alloc.h"
#include <vulkan/vulkan_core.h>

MemoryManager* MemoryManager::memory_manager;

MemoryManager::MemoryManager() { memory_manager = this; }

MemoryManager::~MemoryManager() {
    for (auto image : _images) {
        vmaDestroyImage(Device::device->vma_allocator(), image.second.vk_image, image.second.allocation);
    }
}

MemoryManager::Image MemoryManager::create_image(std::string name, uint32_t width, uint32_t height, uint32_t mipLevels,
                                                 VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling,
                                                 VkImageUsageFlags usage, VkMemoryPropertyFlags properties) {
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
    check_result(vmaCreateImage(Device::device->vma_allocator(), &image_info, &alloc_info, &image.vk_image, &image.allocation, nullptr),
                 "failed to create image!");
    Device::device->set_debug_name(VK_OBJECT_TYPE_IMAGE, (uint64_t)image.vk_image, name);
    _images.insert({name, image});
    return image;
}

void MemoryManager::delete_image(std::string name) {
    Image image = get_image(name);
    vmaDestroyImage(Device::device->vma_allocator(), image.vk_image, image.allocation);
    _images.erase(name);
};