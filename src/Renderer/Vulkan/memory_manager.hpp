#ifndef BUFFERS_H_
#define BUFFERS_H_

#include "vk_mem_alloc.h"
#include <string>
#include <unordered_map>
#include <vulkan/vulkan_core.h>

class MemoryManager {
  public:
    MemoryManager();
    ~MemoryManager();

    static MemoryManager* memory_manager;

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
    std::unordered_map<std::string, Image> _images;
};

#endif // MemoryManager_H_
