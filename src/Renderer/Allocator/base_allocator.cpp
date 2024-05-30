#include "base_allocator.hpp"
#include "../Vulkan/command_runner.hpp"
#include "../Vulkan/common.hpp"
#include <cstddef>
#include <cstring>
#include <iostream>
#include <vulkan/vulkan_core.h>

CPUAllocator::CPUAllocator(size_t const& byte_size) { _base_alloc = alloc(byte_size); }
CPUAllocator::~CPUAllocator() { free(_base_alloc); }

CPUAllocation CPUAllocator::alloc(size_t const& byte_size) {
    CPUAllocation allocation;
    allocation.size_ = byte_size;
    allocation.data = new char[byte_size];
    return allocation;
}

void CPUAllocator::free(CPUAllocation const& allocation) { delete[] allocation.data; }

void CPUAllocator::copy(SubAllocation const& dst_allocation, SubAllocation const& src_allocation, size_t const& byte_size) {
    std::memcpy(_base_alloc.data + dst_allocation.offset, _base_alloc.data + src_allocation.offset, byte_size);
}

void CPUAllocator::write(SubAllocation const& allocation, const void* data, size_t const& byte_size) {
    std::lock_guard<std::mutex> lock(_realloc_lock);
    std::memcpy(_base_alloc.data + allocation.offset, data, byte_size);
}

void CPUAllocator::get(void* dst, SubAllocation const& src_allocation, size_t const& byte_size) {
    std::lock_guard<std::mutex> lock(_realloc_lock);
    std::memcpy(dst, _base_alloc.data + src_allocation.offset, byte_size);
}

void CPUAllocator::copy(CPUAllocation const& dst_allocation, CPUAllocation const& src_allocation, size_t const& byte_size) {
    std::memcpy(dst_allocation.data, src_allocation.data, byte_size);
}

GPUAllocator::GPUAllocator(VkDeviceSize byte_size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, std::string name)
    : _name(name) {
    _buffer_info.usage = usage;

    _alloc_info.requiredFlags = properties;
    _alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
    // TODO
    // Determine this at runtime
    // SEQUENTIAL_WRITE can be faster if used here
    _alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;

    _base_alloc = alloc(byte_size);

    if (_buffer_info.usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
        _addr_info.range = _buffer_info.size;
        // TODO
        // Find out what format should be
        _addr_info.format = VK_FORMAT_UNDEFINED;
        if (_buffer_info.usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) {
            _descriptor_data.emplace(VkDescriptorDataEXT{.pStorageBuffer = &_addr_info});
        } else if (_buffer_info.usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) {
            _descriptor_data.emplace(VkDescriptorDataEXT{.pUniformBuffer = &_addr_info});
        }
    }
}

GPUAllocator::~GPUAllocator() { free(_base_alloc); }

GPUAllocation GPUAllocator::alloc(size_t const& byte_size) {
    GPUAllocation gpu_allocation{};
    _buffer_info.size = byte_size;
    check_result(vmaCreateBuffer(Device::device->vma_allocator(), &_buffer_info, &_alloc_info, &gpu_allocation.buffer,
                                 &gpu_allocation.vma_allocation, nullptr),
                 "failed to create buffer " + _name);
    Device::device->set_debug_name(VK_OBJECT_TYPE_BUFFER, (uint64_t)gpu_allocation.buffer, _name);

    // NOTE:
    // vk_device() should be the same as what was used to create the allocator
    // if it has the usage, then this should be non-zero
    if (_buffer_info.usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
        VkBufferDeviceAddressInfo info{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
        info.buffer = gpu_allocation.buffer;
        _addr_info.address = vkGetBufferDeviceAddress(Device::device->vk_device(), &info);
    }
    // FIXME:
    // update descriptor buffers with new address info

    return gpu_allocation;
}

void GPUAllocator::free(GPUAllocation const& allocation) {
    vmaDestroyBuffer(Device::device->vma_allocator(), allocation.buffer, allocation.vma_allocation);
}

void GPUAllocator::copy(SubAllocation const& dst_allocation, SubAllocation const& src_allocation, size_t const& byte_size) {
    VkBufferCopy buffer_copy{};
    buffer_copy.dstOffset = dst_allocation.offset;
    buffer_copy.srcOffset = src_allocation.offset;
    buffer_copy.size = byte_size;
    std::vector<VkBufferCopy> buffer_copies;
    buffer_copies.push_back(buffer_copy);
    CommandRunner command_runner;
    command_runner.copy_buffer(_base_alloc.buffer, _base_alloc.buffer, buffer_copies);
    command_runner.run();
}

void GPUAllocator::write(SubAllocation const& dst_allocation, const void* data, size_t const& byte_size) {
    std::lock_guard<std::mutex> lock(_realloc_lock);
    vmaCopyMemoryToAllocation(Device::device->vma_allocator(), data, _base_alloc.vma_allocation, dst_allocation.offset, byte_size);
}

void GPUAllocator::get(void* dst, SubAllocation const& src_allocation, size_t const& byte_size) {
    std::lock_guard<std::mutex> lock(_realloc_lock);
    vmaCopyAllocationToMemory(Device::device->vma_allocator(), _base_alloc.vma_allocation, src_allocation.offset, dst, byte_size);
}

void GPUAllocator::copy(GPUAllocation const& dst_allocation, GPUAllocation const& src_allocation, size_t const& byte_size) {
    VkBufferCopy buffer_copy{0, 0, byte_size};
    std::vector<VkBufferCopy> buffer_copies;
    buffer_copies.push_back(buffer_copy);
    CommandRunner command_runner;
    command_runner.copy_buffer(dst_allocation.buffer, src_allocation.buffer, buffer_copies);
    command_runner.run();
}
