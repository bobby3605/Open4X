#include "buffer.hpp"
#include "command_runner.hpp"
#include "common.hpp"
#include <cstring>
#include <stdexcept>

Buffer::Buffer(VmaAllocator& allocator, uint32_t element_size, uint32_t capacity, VkBufferUsageFlags usage,
               VkMemoryPropertyFlags properties)
    : _allocator(allocator), _element_size(element_size), _capacity(capacity) {
    _buffer_info.size = _element_size * capacity;
    _buffer_info.usage = usage;

    if (_buffer_info.size == 0) {
        throw std::runtime_error("error creating a buffer with 0 size");
    }

    _alloc_info.requiredFlags = properties;
    _alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

    check_result(vmaCreateBuffer(allocator, &_buffer_info, &_alloc_info, &_vk_buffer, &_allocation, nullptr), "failed to create buffer!");
}
void Buffer::unmap(VmaAllocation& allocation) {
    vmaUnmapMemory(_allocator, allocation);
    _mapped = false;
}
Buffer::~Buffer() {
    if (_mapped) {
        unmap(_allocation);
    }
    vmaDestroyBuffer(_allocator, _vk_buffer, _allocation);
}
void Buffer::map(VmaAllocation& allocation, void* data) {
    vmaMapMemory(_allocator, allocation, &data);
    _mapped = true;
}
void Buffer::push_back(void* item) {
    if (!_mapped) {
        map(_allocation, _data);
    }
    if (_size == _capacity) {
        resize(_capacity + realloc_size);
    }
    std::memcpy(_data, item, _element_size);
}
void Buffer::resize(uint32_t new_size) {
    /* NOTE:
     * Validation layer can handle this better
    if (!check_usage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT)) {
        throw std::runtime_error("error: tried to copy buffer without transfer_src set");
    }
    */
    VkBufferCreateInfo buffer_info = _buffer_info;
    buffer_info.size = _element_size * new_size;
    buffer_info.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VkBuffer new_buffer;
    VmaAllocation new_allocation;

    check_result(vmaCreateBuffer(_allocator, &buffer_info, &_alloc_info, &new_buffer, &new_allocation, nullptr),
                 "failed to create buffer!");

    copy(new_buffer, _element_size * std::min(_size, new_size));

    _vk_buffer = new_buffer;
    _allocation = new_allocation;
    _capacity = new_size;
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
