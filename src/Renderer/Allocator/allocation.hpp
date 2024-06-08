#ifndef ALLOCATION_H_
#define ALLOCATION_H_
#include "../Vulkan/device.hpp"
#include "vk_mem_alloc.h"
#include <cstddef>
#include <vulkan/vulkan_core.h>

template <typename AllocationT> class Allocation {
  public:
    virtual ~Allocation(){};
    virtual size_t const& size() const;
    virtual void realloc(size_t const& byte_size);

  protected:
    virtual void get(void* dst, size_t const& src_offset, size_t const& byte_size) = 0;
    virtual void write(size_t const& dst_offset, const void* src_data, size_t const& byte_size) = 0;
    virtual void copy(AllocationT* dst_allocation, size_t const& dst_offset, size_t const& src_offset, size_t const& byte_size) = 0;
    virtual void copy(AllocationT* dst_allocation) = 0;
};

template <typename> class LinearAllocator;
template <typename> class FixedAllocator;
template <typename> class ContiguousFixedAllocator;
template <typename> class VoidAllocator;

template <template <class> class AllocatorT, typename ParentAllocationT>
class SubAllocation : Allocation<SubAllocation<AllocatorT, ParentAllocationT>> {
  public:
    SubAllocation(size_t const& offset, size_t const& size, AllocatorT<ParentAllocationT>* allocator, ParentAllocationT* parent)
        : _offset(offset), _size(size), _parent(parent), _allocator(allocator) {}
    SubAllocation(size_t const& offset, size_t const& size, ParentAllocationT* parent) : _offset(offset), _size(size), _parent(parent) {}
    void get(void* dst) { get(dst, 0, _size); }
    void write(const void* data) { write(0, data, _size); }
    void copy(SubAllocation<AllocatorT, ParentAllocationT>* dst_allocation) { copy(dst_allocation, 0, 0, _size); }
    void realloc(size_t const& byte_size) {
        // NOTE:
        // _allocator must support sized reallocs
        SubAllocation<AllocatorT, ParentAllocationT>* tmp_alloc = _allocator->alloc(byte_size);
        copy(tmp_alloc);
        _offset = tmp_alloc->_offset;
        _size = tmp_alloc->_size;
        delete tmp_alloc;
    }

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
    AllocatorT<ParentAllocationT>* _allocator = nullptr;
    void get(void* dst, size_t const& src_offset, size_t const& byte_size) { _parent->get(dst, src_offset + _offset, byte_size); }
    void write(size_t const& dst_offset, const void* src_data, size_t const& byte_size) {
        _parent->write(dst_offset + _offset, src_data, byte_size);
    }
    void copy(SubAllocation<AllocatorT, ParentAllocationT>* dst_allocation, size_t const& dst_offset, size_t const& src_offset,
              size_t const& byte_size) {
        _parent->copy(dst_allocation->_parent, dst_offset + dst_allocation->_offset, src_offset + _offset, byte_size);
    }
    friend class SubAllocation<LinearAllocator, SubAllocation<AllocatorT, ParentAllocationT>>;
    friend class SubAllocation<FixedAllocator, SubAllocation<AllocatorT, ParentAllocationT>>;
    friend class SubAllocation<ContiguousFixedAllocator, SubAllocation<AllocatorT, ParentAllocationT>>;
    friend class SubAllocation<VoidAllocator, SubAllocation<AllocatorT, ParentAllocationT>>;

    /* FIXME:
     * Free block from parent
     void free() {}
    */
};

namespace std {
template <template <class> class AT, typename PT> struct hash<SubAllocation<AT, PT>> {
    size_t operator()(SubAllocation<AT, PT> const& allocation) const {
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

    template <template <class> class, typename> friend class SubAllocation;
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

    template <template <class> class, typename> friend class SubAllocation;
};

#endif // ALLOCATION_H_
