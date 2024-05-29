#include "buffer.hpp"
#include "command_runner.hpp"
#include "common.hpp"
#include "device.hpp"
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
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

    create(_buffer_info, _alloc_info, _vk_buffer, _allocation, name);

    if (_buffer_info.usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
        _addr_info.range = _buffer_info.size;
        // TODO
        // Find out what format should be
        _addr_info.format = VK_FORMAT_UNDEFINED;
        if (_buffer_info.usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) {
            _descriptor_data.pStorageBuffer = &_addr_info;
            _has_descriptor_data = true;
        } else if (_buffer_info.usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) {
            _descriptor_data.pUniformBuffer = &_addr_info;
            _has_descriptor_data = true;
        }
    }
}

VkDescriptorDataEXT const& Buffer::descriptor_data() const {
    if (_has_descriptor_data) {
        return _descriptor_data;
    } else {
        throw std::runtime_error("tried to get descriptor data on buffer with unknown usage: " + _name + " " +
                                 std::to_string(_buffer_info.usage));
    }
}

void Buffer::create(VkBufferCreateInfo& buffer_info, VmaAllocationCreateInfo& alloc_info, VkBuffer& buffer, VmaAllocation& allocation,
                    std::string& name) {

    check_result(vmaCreateBuffer(_allocator, &buffer_info, &alloc_info, &buffer, &allocation, nullptr), "failed to create buffer " + name);
    Device::device->set_debug_name(VK_OBJECT_TYPE_BUFFER, (uint64_t)buffer, name);
    // NOTE:
    // vk_device() should be the same as what was used to create the allocator
    // if it has the usage, then this should be non-zero
    if (buffer_info.usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
        VkBufferDeviceAddressInfo info{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
        info.buffer = buffer;
        _addr_info.address = vkGetBufferDeviceAddress(Device::device->vk_device(), &info);
    }
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
Buffer::~Buffer() { destroy(); }

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

bool inline Buffer::check_usage(VkBufferUsageFlags usage) {
    if ((_buffer_info.usage & usage) == usage) {
        return true;
    } else {
        return false;
    }
}

void Buffer::copy(VkBuffer dst, VkDeviceSize copy_size) { copy(dst, 0, 0, copy_size); }

void Buffer::copy(VkBuffer dst, VkDeviceSize src_offset, VkDeviceSize dst_offset, VkDeviceSize copy_size) {
    VkBufferCopy buffer_copy{src_offset, dst_offset, copy_size};
    std::vector<VkBufferCopy> buffer_copies;
    buffer_copies.push_back(buffer_copy);
    CommandRunner command_runner;
    command_runner.copy_buffer(_vk_buffer, dst, buffer_copies);
    command_runner.run();
}

const VkDeviceAddress& Buffer::device_address() const {
    if ((_buffer_info.usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) != VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
        throw std::runtime_error("tried to get device address on buffer without flag: " + _name);
    } else {
        return _addr_info.address;
    }
};
