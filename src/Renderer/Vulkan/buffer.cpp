#include "buffer.hpp"
#include "command_runner.hpp"
#include "common.hpp"
#include "device.hpp"
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

Buffer::Buffer(VmaAllocator& allocator, VkDeviceSize byte_size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
               std::string name)
    : _allocator(allocator), _name(name) {
    if (byte_size == 0) {
        throw std::runtime_error("error creating a buffer with 0 size! " + _name);
    }

    _buffer_info.size = byte_size;
    _buffer_info.usage = usage;

    _alloc_info.requiredFlags = properties;
    _alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
    // TODO
    // Determine this at runtime
    // SEQUENTIAL_WRITE can be faster if used here
    _alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;

    check_result(vmaCreateBuffer(allocator, &_buffer_info, &_alloc_info, &_vk_buffer, &_allocation, nullptr),
                 "failed to create buffer " + name);
    Device::device->set_debug_name(VK_OBJECT_TYPE_BUFFER, (uint64_t)vk_buffer(), name);
    // NOTE:
    // vk_device() should be the same as what was used to create the allocator
    // if it has the usage, then this should be non-zero
    if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
        VkBufferDeviceAddressInfo info{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
        info.buffer = vk_buffer();
        _device_address = vkGetBufferDeviceAddress(Device::device->vk_device(), &info);
    }

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
char* Buffer::data() {
    if (!_mapped) {
        map();
    }
    return reinterpret_cast<char*>(_data);
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
    Device::device->set_debug_name(VK_OBJECT_TYPE_BUFFER, (uint64_t)vk_buffer(), _name);

    if (new_buffer_info.usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
        VkBufferDeviceAddressInfo info{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
        info.buffer = vk_buffer();
        _device_address = vkGetBufferDeviceAddress(Device::device->vk_device(), &info);
    }

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

const VkDeviceAddress& Buffer::device_address() const {
    if (_device_address == -1) {
        throw std::runtime_error("tried to get device address on buffer without flag: " + _name);
    } else {
        return _device_address;
    }
};
