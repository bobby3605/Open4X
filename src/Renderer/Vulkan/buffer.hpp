#ifndef BUFFER_H_
#define BUFFER_H_

#include "vk_mem_alloc.h"
#include <vulkan/vulkan_core.h>

class Buffer {
  public:
    Buffer(VmaAllocator& allocator, uint32_t element_size, uint32_t capacity, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    ~Buffer();
    void unmap(VmaAllocation& allocation);
    void map(VmaAllocation& allocation, void* data);
    uint32_t realloc_size = 1;
    void push_back(void* item);
    void resize(uint32_t new_size);
    bool inline check_usage(VkBufferUsageFlags usage);
    void copy(VkBuffer dst, VkDeviceSize copy_size);
    void* data() { return _data; }
    VkBuffer vk_buffer() { return _vk_buffer; }

  private:
    VkBuffer _vk_buffer;
    VmaAllocation _allocation;
    VkBufferCreateInfo _buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    VmaAllocationCreateInfo _alloc_info = {};
    uint32_t _element_size;
    uint32_t _size = 0;
    uint32_t _capacity = 0;
    bool _mapped = false;
    void* _data;
    VmaAllocator _allocator;
};

#endif // BUFFER_H_
