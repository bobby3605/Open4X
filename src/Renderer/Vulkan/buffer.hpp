#ifndef BUFFER_H_
#define BUFFER_H_

#include "vk_mem_alloc.h"
#include <cstdlib>
#include <string>
#include <vulkan/vulkan_core.h>

class Buffer {
  public:
    Buffer(VmaAllocator& allocator, VkDeviceSize byte_size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, std::string name);
    ~Buffer();
    void unmap();
    void map();
    void resize(std::size_t new_size);
    void copy(VkBuffer dst, VkDeviceSize src_offset, VkDeviceSize dst_offset, VkDeviceSize copy_size);
    void copy(VkBuffer dst, VkDeviceSize copy_size);
    char* data();
    const VkBuffer& vk_buffer() { return _vk_buffer; }
    // return true if resized
    bool alloc(std::size_t const& byte_size, VmaVirtualAllocation& alloc, VkDeviceSize& offset, VkDeviceSize alignment = 1);
    void realloc(VkDeviceSize const& new_byte_size, VmaVirtualAllocation& alloc, VkDeviceSize& offset, VkDeviceSize alignment = 1);
    void free(VmaVirtualAllocation& alloc);
    const VkDeviceAddress& device_address() const;
    VkDeviceSize size() const { return _buffer_info.size; }
    VkBufferCreateInfo buffer_info() const { return _buffer_info; }
    VkDescriptorDataEXT const& descriptor_data() const;

  private:
    VmaAllocator _allocator;
    VkBuffer _vk_buffer;
    VmaAllocation _allocation;
    VkBufferCreateInfo _buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    VmaAllocationCreateInfo _alloc_info = {};
    bool _mapped = false;
    // TODO
    // Delete =, so that resize can be memory safe
    // Maybe unique_ptr?
    void* _data;
    void create(VkBufferCreateInfo& buffer_info, VmaAllocationCreateInfo& alloc_info, VkBuffer& buffer, VmaAllocation& allocation,
                std::string& name);
    void destroy();
    bool inline check_usage(VkBufferUsageFlags usage);
    VmaVirtualBlock _virtual_block;
    std::string _name;
    VkDescriptorDataEXT _descriptor_data;
    bool _has_descriptor_data = false;
    VkDescriptorAddressInfoEXT _addr_info{VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT};
};

#endif // BUFFER_H_
