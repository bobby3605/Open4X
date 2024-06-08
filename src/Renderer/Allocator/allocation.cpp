#include "allocation.hpp"
#include "../Vulkan/command_runner.hpp"
#include "../Vulkan/common.hpp"
#include <cstring>
#include <iostream>
#include <vulkan/vulkan_core.h>

void CPUAllocation::alloc(size_t const& byte_size) {
    _size = byte_size;
    _data = new char[byte_size];
}

void CPUAllocation::free() {
    if (_data != nullptr) {
        delete _data;
    }
}

CPUAllocation::~CPUAllocation() { free(); }

void CPUAllocation::get(void* dst, size_t const& offset, size_t const& byte_size) {
    //    std::lock_guard<std::mutex> lock(_realloc_lock);
    std::memcpy(dst, _data + offset, byte_size);
}

void CPUAllocation::write(size_t const& dst_offset, const void* src_data, size_t const& byte_size) {
    if (_size < dst_offset + byte_size) {
        std::cout << "shouldn't be here, cpu write realloc" << std::endl;
    }
    //    std::lock_guard<std::mutex> lock(_realloc_lock);
    std::memcpy(_data + dst_offset, src_data, byte_size);
}

void CPUAllocation::copy(CPUAllocation* dst_allocation, size_t const& dst_offset, size_t const& src_offset, size_t const& byte_size) {
    std::memcpy(dst_allocation->_data + dst_offset, _data + src_offset, byte_size);
}

void CPUAllocation::copy(CPUAllocation* dst_allocation) { std::memcpy(dst_allocation->_data, _data, _size); }

void CPUAllocation::realloc(size_t const& byte_size) {
    //        std::lock_guard<std::mutex> lock(_realloc_lock);
    CPUAllocation new_alloc = CPUAllocation();
    new_alloc.alloc(byte_size);
    // Only copy if current has been alloced
    if (_data != VK_NULL_HANDLE) {
        // TODO
        // support shrinking
        copy(&new_alloc);
    }
    // free current allocation
    free();
    // move new allocation to current
    _data = new_alloc._data;
    _size = new_alloc._size;
    // bypass destructor free
    new_alloc._data = nullptr;
}

GPUAllocation::GPUAllocation(VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, std::string const& name) : _name(name) {
    // TODO
    // Find out if there are any performance implications for doing this
    _buffer_info.usage =
        usage | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    // Set alloc info
    _alloc_info.requiredFlags = properties;
    _alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
    // TODO
    // Determine this at runtime
    // SEQUENTIAL_WRITE can be faster if used here
    _alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;

    if (_buffer_info.usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
        _addr_info.range = _buffer_info.size;
        // TODO
        // Find out what format should be
        _addr_info.format = VK_FORMAT_UNDEFINED;
        if (_buffer_info.usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) {
            _descriptor_data = {.pStorageBuffer = &_addr_info};
        } else if (_buffer_info.usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) {
            _descriptor_data = {.pUniformBuffer = &_addr_info};
        } else {
            std::cout << "warning: unknown buffer usage for descriptor data: " << _name << ", " << _buffer_info.usage << std::endl;
        }
    }
}

void GPUAllocation::alloc(size_t const& byte_size) {
    _buffer_info.size = byte_size;
    check_result(vmaCreateBuffer(Device::device->vma_allocator(), &_buffer_info, &_alloc_info, &_buffer, &_vma_allocation, nullptr),
                 "failed to create buffer " + _name);
    Device::device->set_debug_name(VK_OBJECT_TYPE_BUFFER, (uint64_t)_buffer, _name);
    std::cout << "gpu alloc: " << _name << std::endl;

    // NOTE:
    // vk_device() should be the same as what was used to create the allocator
    // if it has the usage, then this should be non-zero
    if (_buffer_info.usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
        VkBufferDeviceAddressInfo info{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
        info.buffer = _buffer;
        _addr_info.address = vkGetBufferDeviceAddress(Device::device->vk_device(), &info);
    }
}

void GPUAllocation::free() {
    if (_buffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(Device::device->vma_allocator(), _buffer, _vma_allocation);
    }
}

GPUAllocation::~GPUAllocation() { free(); }

void GPUAllocation::get(void* dst, size_t const& offset, size_t const& byte_size) {
    //   std::lock_guard<std::mutex> lock(_realloc_lock);
    vmaCopyAllocationToMemory(Device::device->vma_allocator(), _vma_allocation, offset, dst, byte_size);
}

void GPUAllocation::write(size_t const& dst_offset, const void* src_data, size_t const& byte_size) {
    if (_buffer_info.size < (dst_offset + byte_size)) {
        std::cout << "write realloc, should never be here, buffer: " << _name << std::endl;
    }
    //    std::lock_guard<std::mutex> lock(_realloc_lock);
    vmaCopyMemoryToAllocation(Device::device->vma_allocator(), src_data, _vma_allocation, dst_offset, byte_size);
}

void GPUAllocation::copy(GPUAllocation* dst_allocation, size_t const& dst_offset, size_t const& src_offset, size_t const& byte_size) {
    VkBufferCopy buffer_copy{};
    buffer_copy.dstOffset = dst_offset;
    buffer_copy.srcOffset = src_offset;
    buffer_copy.size = byte_size;
    std::vector<VkBufferCopy> buffer_copies;
    buffer_copies.push_back(buffer_copy);
    CommandRunner command_runner;
    command_runner.copy_buffer(dst_allocation->_buffer, _buffer, buffer_copies);
    command_runner.run();
}

void GPUAllocation::copy(GPUAllocation* dst_allocation) {
    VkBufferCopy buffer_copy{0, 0, VK_WHOLE_SIZE};
    std::vector<VkBufferCopy> buffer_copies;
    buffer_copies.push_back(buffer_copy);
    CommandRunner command_runner;
    command_runner.copy_buffer(dst_allocation->_buffer, _buffer, buffer_copies);
    command_runner.run();
}

void GPUAllocation::realloc(size_t const& byte_size) {
    //        std::lock_guard<std::mutex> lock(_realloc_lock);
    GPUAllocation new_alloc = GPUAllocation(_buffer_info.usage, _alloc_info.flags, _name);
    new_alloc.alloc(byte_size);
    // Only copy if current has been alloced
    if (_buffer != VK_NULL_HANDLE) {
        copy(&new_alloc);
    }
    // free current allocation
    free();
    // move new allocation to current
    // NOTE:
    // This might be done better with a move constructor
    _buffer = new_alloc._buffer;
    _buffer_info.size = byte_size;
    // NOTE:
    // bypass free in destructor
    new_alloc._buffer = VK_NULL_HANDLE;
    _vma_allocation = new_alloc._vma_allocation;
    _addr_info = new_alloc._addr_info;
    // NOTE:
    // only _buffer, _vma_allocation, and _addr_info need to be copied/moved
}
