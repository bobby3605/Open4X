#include "buffer.hpp"
#include "command_runner.hpp"
#include "common.hpp"
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

Buffer::Buffer(VmaAllocator& allocator, VkDeviceSize byte_size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
    : _allocator(allocator) {
    if (byte_size == 0) {
        throw std::runtime_error("error creating a buffer with 0 size!");
    }

    _buffer_info.size = byte_size;
    _buffer_info.usage = usage;

    _alloc_info.requiredFlags = properties;
    _alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

    check_result(vmaCreateBuffer(allocator, &_buffer_info, &_alloc_info, &_vk_buffer, &_allocation, nullptr), "failed to create buffer!");

    VmaVirtualBlockCreateInfo virtual_block_create_info{};
    // Allocate max size in the virtual block
    // This allows for the underlying VkBuffer to be resized,
    // without disrupting the virtual allocations
    virtual_block_create_info.size = -1;
    check_result(vmaCreateVirtualBlock(&virtual_block_create_info, &_virtual_block), "virtual block creation failed, shouldn't be here!");
}
void Buffer::unmap() {
    vmaUnmapMemory(_allocator, _allocation);
    _mapped = false;
}
void Buffer::destroy() {
    if (_mapped) {
        unmap();
    }
    vmaDestroyBuffer(_allocator, _vk_buffer, _allocation);
}
Buffer::~Buffer() {
    vmaClearVirtualBlock(_virtual_block);
    vmaDestroyVirtualBlock(_virtual_block);
    destroy();
}

void Buffer::map() {
    vmaMapMemory(_allocator, _allocation, &_data);
    _mapped = true;
}
// TODO
// Thread safety with element access
void Buffer::resize(std::size_t new_byte_size) {
    if (!check_usage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT)) {
        throw std::runtime_error("error: tried to copy buffer without transfer_src set");
    }

    //    std::cout << "resizing to size: " << new_byte_size << std::endl;
    VkBufferCreateInfo new_buffer_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    new_buffer_info.size = new_byte_size;
    new_buffer_info.usage = _buffer_info.usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VkBuffer new_buffer;
    VmaAllocation new_allocation;

    check_result(vmaCreateBuffer(_allocator, &new_buffer_info, &_alloc_info, &new_buffer, &new_allocation, nullptr),
                 "failed to create buffer!");

    copy(new_buffer, std::min(_buffer_info.size, new_buffer_info.size));
    bool tmp_mapped = _mapped;
    // destroy buffer and allocation before overwriting vars
    destroy();

    _vk_buffer = new_buffer;
    _buffer_info = new_buffer_info;
    _allocation = new_allocation;
    if (tmp_mapped)
        map();
}
bool inline Buffer::check_usage(VkBufferUsageFlags usage) {
    if ((_buffer_info.usage & usage) == usage) {
        return true;
    } else {
        return false;
    }
}
void Buffer::copy(VkBuffer dst, VkDeviceSize copy_size) {
    VkBufferCopy buffer_copy{0, 0, copy_size};
    std::vector<VkBufferCopy> buffer_copies;
    buffer_copies.push_back(buffer_copy);
    CommandRunner command_runner;
    command_runner.copy_buffer(_vk_buffer, dst, buffer_copies);
    command_runner.run();
}

void Buffer::alloc(std::size_t const& byte_size, VmaVirtualAllocation& alloc, VkDeviceSize& offset) {
    VmaVirtualAllocationCreateInfo alloc_create_info{};
    alloc_create_info.size = byte_size;
    check_result(vmaVirtualAllocate(_virtual_block, &alloc_create_info, &alloc, &offset),
                 "failed to allocate inside virtual block, shouldn't be here!");
    if (offset + byte_size > _buffer_info.size) {
        resize(offset + byte_size);
    }
}

void Buffer::free(VmaVirtualAllocation& alloc) { vmaVirtualFree(_virtual_block, alloc); }
