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

// TODO
// Thread safety with element access
void Buffer::resize(std::size_t new_byte_size) {
    if (!check_usage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT)) {
        throw std::runtime_error("error: tried to copy buffer without transfer_src set on buffer: " + _name);
    }

    //    std::cout << "resizing to size: " << new_byte_size << std::endl;
    VkBufferCreateInfo new_buffer_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    new_buffer_info.size = new_byte_size;
    new_buffer_info.usage = _buffer_info.usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VkBuffer new_buffer;
    VmaAllocation new_allocation;

    create(new_buffer_info, _alloc_info, new_buffer, new_allocation, _name);

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

void Buffer::copy(VkBuffer dst, VkDeviceSize copy_size) { copy(dst, 0, 0, copy_size); }

void Buffer::copy(VkBuffer dst, VkDeviceSize src_offset, VkDeviceSize dst_offset, VkDeviceSize copy_size) {
    VkBufferCopy buffer_copy{src_offset, dst_offset, copy_size};
    std::vector<VkBufferCopy> buffer_copies;
    buffer_copies.push_back(buffer_copy);
    CommandRunner command_runner;
    command_runner.copy_buffer(_vk_buffer, dst, buffer_copies);
    command_runner.run();
}

bool Buffer::alloc(std::size_t const& byte_size, VmaVirtualAllocation& alloc, VkDeviceSize& offset, VkDeviceSize alignment) {
    VmaVirtualAllocationCreateInfo alloc_create_info{};
    alloc_create_info.size = byte_size;
    alloc_create_info.alignment = alignment;
    check_result(vmaVirtualAllocate(_virtual_block, &alloc_create_info, &alloc, &offset),
                 "failed to allocate inside virtual block, shouldn't be here!");
    if (align(offset + byte_size, alignment) > _buffer_info.size) {
        resize(align(offset + byte_size, alignment));
        return true;
    }
    return false;
}

void Buffer::realloc(VkDeviceSize const& new_byte_size, VmaVirtualAllocation& alloc, VkDeviceSize& offset, VkDeviceSize alignment) {
    if (alloc != VK_NULL_HANDLE) {
        // get current alloc size
        VmaVirtualAllocationInfo alloc_info;
        vmaGetVirtualAllocationInfo(_virtual_block, alloc, &alloc_info);
        VkDeviceSize curr_byte_size = alloc_info.size;
        // TODO
        // shrink allocation if the size difference is large
        if (curr_byte_size > new_byte_size) {
            VmaVirtualAllocation new_alloc;
            VkDeviceSize new_offset;
            // create new allocation
            this->alloc(new_byte_size, new_alloc, new_offset, alignment);
            // get passed alloc size
            // copy old data to new allocation
            copy(vk_buffer(), offset, new_offset, curr_byte_size);
            // free old allocation
            free(alloc);

            // set passed alloc and offset to new alloc and offset
            alloc = new_alloc;
            offset = new_offset;
        }
    } else {
        // If null allocation, just pass to alloc
        this->alloc(new_byte_size, alloc, offset, alignment);
    }
}

void Buffer::free(VmaVirtualAllocation& alloc) { vmaVirtualFree(_virtual_block, alloc); }

const VkDeviceAddress& Buffer::device_address() const {
    if ((_buffer_info.usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) != VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
        throw std::runtime_error("tried to get device address on buffer without flag: " + _name);
    } else {
        return _addr_info.address;
    }
};
