#ifndef BUFFERS_H_
#define BUFFERS_H_

#include "buffer.hpp"
#include "vk_mem_alloc.h"
#include <string>
#include <unordered_map>
#include <vulkan/vulkan_core.h>

class MemoryManager {
  public:
    MemoryManager();
    ~MemoryManager();

    static MemoryManager* memory_manager;

    Buffer* create_buffer(std::string name, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    Buffer* get_buffer(std::string name);
    void delete_buffer(std::string name);
    bool buffer_exists(std::string name) const { return _buffers.count(name) == 1; }
    // NOTE:
    // If a buffer is being overwritten,
    // it must be deleted afterwards,
    // or else there will be a memory leak
    void set_buffer(std::string const& name, Buffer* buffer);
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
    std::unordered_map<std::string, Buffer*> _buffers;
    std::unordered_map<std::string, Image> _images;
};

#endif // MemoryManager_H_
