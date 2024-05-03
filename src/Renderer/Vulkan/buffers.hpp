#ifndef BUFFERS_H_
#define BUFFERS_H_

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include "vk_mem_alloc.h"
#include <string>
#include <unordered_map>

class Buffers {
  public:
    Buffers();
    ~Buffers();

    static Buffers* buffers;

    struct Buffer {
        VkBuffer vk_buffer;
        VmaAllocation allocation;
    };
    void create_buffer(std::string name, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    Buffer get_buffer(std::string name) const { return _buffers.at(name); };
    void delete_buffer(std::string name);

    struct Image {
        VkImage vk_image;
        VmaAllocation allocation;
    };
    void create_image(std::string name, uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples,
                      VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
    Image get_image(std::string name) const { return _images.at(name); };
    void delete_image(std::string name);

  private:
    VmaAllocator _allocator;
    std::unordered_map<std::string, Buffer> _buffers;
    std::unordered_map<std::string, Image> _images;
};

#endif // BUFFERS_H_
