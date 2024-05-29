#ifndef BUFFERS_H_
#define BUFFERS_H_

#include "../Allocator/base_allocator.hpp"
#include "vk_mem_alloc.h"
#include <string>
#include <unordered_map>
#include <vulkan/vulkan_core.h>

class MemoryManager {
  public:
    MemoryManager();
    ~MemoryManager();

    static MemoryManager* memory_manager;

    GPUAllocator* create_gpu_allocator(std::string name, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    GPUAllocator* get_gpu_allocator(std::string name);
    void delete_gpu_allocator(std::string name);
    bool gpu_allocator_exists(std::string name) const { return _gpu_allocators.count(name) == 1; }
    // NOTE:
    // If a buffer is being overwritten,
    // it must be deleted afterwards,
    // or else there will be a memory leak
    void set_gpu_allocator(std::string const& name, GPUAllocator* buffer);
    VmaAllocator allocator() { return _allocator; }

    struct Image {
        VkImage vk_image;
        VmaAllocation allocation;
    };
    Image create_image(std::string name, uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples,
                       VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
    Image get_image(std::string name) const { return _images.at(name); };
    void delete_image(std::string name);

  private:
    VmaAllocator _allocator;
    std::unordered_map<std::string, GPUAllocator*> _gpu_allocators;
    std::unordered_map<std::string, Image> _images;
};

#endif // MemoryManager_H_
