#ifndef ALLOCATION_H_
#define ALLOCATION_H_
#include "../Vulkan/device.hpp"
#include "vk_mem_alloc.h"
#include <cstddef>
#include <vulkan/vulkan_core.h>

template <typename AllocationT> class Allocation {
  public:
    virtual size_t const& size() const;

  protected:
    virtual void get(void* dst, size_t const& src_offset, size_t const& byte_size) = 0;
    virtual void write(size_t const& dst_offset, const void* src_data, size_t const& byte_size) = 0;
    virtual void copy(AllocationT* dst_allocation, size_t const& dst_offset, size_t const& src_offset, size_t const& byte_size) = 0;
    virtual void copy(AllocationT* dst_allocation) = 0;
};

template <typename ParentAllocationT> class SubAllocation : Allocation<SubAllocation<ParentAllocationT>> {
  public:
    SubAllocation(size_t const& offset, size_t const& size, ParentAllocationT* parent) : _offset(offset), _size(size), _parent(parent) {}
    void get(void* dst) { get(dst, 0, _size); }
    void write(const void* data) { write(0, data, _size); }
    void copy(SubAllocation<ParentAllocationT>* dst_allocation) { copy(dst_allocation, 0, 0, _size); }

    bool operator==(const SubAllocation& other) const {
        return _offset == other._offset && _size == other._size && _parent == other._parent;
    }

    size_t const& size() const { return _size; }
    size_t const& offset() const { return _offset; }

    // NOTE:
    // Offset into the parent
    size_t _offset = 0;
    size_t _size = 0;

  protected:
    ParentAllocationT* _parent = nullptr;
    void get(void* dst, size_t const& src_offset, size_t const& byte_size) { _parent->get(dst, src_offset + _offset, byte_size); }
    void write(size_t const& dst_offset, const void* src_data, size_t const& byte_size) {
        _parent->write(dst_offset + _offset, src_data, byte_size);
    }
    void copy(SubAllocation<ParentAllocationT>* dst_allocation, size_t const& dst_offset, size_t const& src_offset,
              size_t const& byte_size) {
        _parent->copy(dst_allocation->_parent, dst_offset + dst_allocation->_offset, src_offset + _offset, byte_size);
    }

    /* FIXME:
     * Free block from parent
     void free() {}
    */
};

namespace std {
template <typename T> struct hash<SubAllocation<T>> {
    size_t operator()(SubAllocation<T> const& allocation) const {
        return hash<size_t>()(allocation._offset) ^ hash<size_t>()(allocation._size);
    }
};
} // namespace std

class CPUAllocation : Allocation<CPUAllocation> {
  public:
    ~CPUAllocation();
    void realloc(const size_t& byte_size);
    size_t const& size() const { return _size; }

  protected:
    friend class CPUAllocator;
    size_t _size = 0;
    char* _data = nullptr;

    void alloc(size_t const& byte_size);
    void free();

    void get(void* dst, size_t const& offset, size_t const& byte_size);
    void write(size_t const& dst_offset, const void* src_data, size_t const& byte_size);
    void copy(CPUAllocation* dst_allocation, size_t const& dst_offset, size_t const& src_offset, size_t const& byte_size);
    void copy(CPUAllocation* dst_allocation);

    friend class SubAllocation<CPUAllocation>;
};

class GPUAllocation : Allocation<GPUAllocation> {
  public:
    GPUAllocation(VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, std::string const& name);
    ~GPUAllocation();
    void realloc(const size_t& byte_size);
    size_t const& size() const { return _buffer_info.size; }
    VmaAllocationCreateInfo const& alloc_info() { return _alloc_info; }
    VkDescriptorDataEXT const& descriptor_data() { return _descriptor_data; }
    VkDescriptorAddressInfoEXT const& addr_info() { return _addr_info; }
    VkBuffer const& buffer() const { return _buffer; }

  protected:
    friend class GPUAllocator;
    VkBuffer _buffer = VK_NULL_HANDLE;
    VkBufferCreateInfo _buffer_info{.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .size = 0};
    VmaAllocation _vma_allocation;
    VmaAllocationCreateInfo _alloc_info;
    std::string _name;
    VkDescriptorDataEXT _descriptor_data;
    VkDescriptorAddressInfoEXT _addr_info{VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT};

    void alloc(size_t const& byte_size);
    void free();

    void get(void* dst, size_t const& offset, size_t const& byte_size);
    void write(size_t const& dst_offset, const void* src_data, size_t const& byte_size);
    void copy(GPUAllocation* dst_allocation, size_t const& dst_offset, size_t const& src_offset, size_t const& byte_size);
    void copy(GPUAllocation* dst_allocation);

    friend class SubAllocation<GPUAllocation>;
};

#endif // ALLOCATION_H_
