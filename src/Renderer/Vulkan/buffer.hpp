#ifndef BUFFER_H_
#define BUFFER_H_

#include "vk_mem_alloc.h"
#include <cstdlib>
#include <vulkan/vulkan_core.h>

class Buffer {
  public:
    Buffer(VmaAllocator& allocator, VkDeviceSize byte_size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    ~Buffer();
    void unmap();
    void map();
    void resize(std::size_t new_size);
    void copy(VkBuffer dst, VkDeviceSize copy_size);
    char* data() { return reinterpret_cast<char*>(_data); }
    const VkBuffer& vk_buffer() { return _vk_buffer; }
    void alloc(std::size_t const& byte_size, VmaVirtualAllocation& alloc, VkDeviceSize& offset);
    void free(VmaVirtualAllocation& alloc);
    const VkDeviceAddress& device_address() { return _device_address; };

  private:
    VmaAllocator _allocator;
    VkBuffer _vk_buffer;
    VkDeviceAddress _device_address;
    VmaAllocation _allocation;
    VkBufferCreateInfo _buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    VmaAllocationCreateInfo _alloc_info = {};
    bool _mapped = false;
    // TODO
    // Delete =, so that resize can be memory safe
    // Maybe unique_ptr?
    void* _data;
    void destroy();
    bool inline check_usage(VkBufferUsageFlags usage);
    VmaVirtualBlock _virtual_block;
};

#endif // BUFFER_H_
