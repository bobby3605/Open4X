#ifndef BASE_ALLOCATOR_H_
#define BASE_ALLOCATOR_H_
#include "../Vulkan/device.hpp"
#include "vk_mem_alloc.h"
#include <cstddef>
#include <mutex>
#include <vulkan/vulkan_core.h>

struct BaseAllocation {
    virtual size_t inline size() = 0;
};

struct SubAllocation : BaseAllocation {
    size_t offset;
    size_t size_;
    size_t inline size() { return size_; };
    bool operator==(const SubAllocation& other) const { return offset == other.offset && size_ == other.size_; }
};

namespace std {
template <> struct hash<SubAllocation> {
    size_t operator()(SubAllocation const& allocation) const {
        return (hash<size_t>()(allocation.offset) ^ (hash<size_t>()(allocation.size_)));
    }
};
} // namespace std

struct CPUAllocation : BaseAllocation {
    char* data;
    size_t size_;
    size_t inline size() { return size_; };
};

struct GPUAllocation : BaseAllocation {
    VkBuffer buffer;
    VmaAllocation vma_allocation;
    size_t inline size() {
        VmaAllocationInfo info{};
        vmaGetAllocationInfo(Device::device->vma_allocator(), vma_allocation, &info);
        return info.size;
    };
};

template <typename AllocationT, typename BaseAllocationT> class Allocator {
    static_assert(std::is_base_of<BaseAllocation, BaseAllocationT>::value, "BaseAllocationT must be a BaseAllocation");

  public:
    typedef AllocationT AllocT;
    typedef BaseAllocationT BaseT;
    virtual AllocationT alloc(size_t const& byte_size) = 0;
    virtual void free(AllocationT const& base_alloc) = 0;
    virtual void copy(AllocationT const& dst_allocation, AllocationT const& src_allocation, size_t const& byte_size) = 0;
    void realloc(AllocationT& allocation, size_t const& byte_size) {
        std::lock_guard<std::mutex> lock(_realloc_lock);
        AllocationT new_alloc = alloc(byte_size);
        copy(new_alloc, allocation, byte_size);
        free(allocation);
        allocation = new_alloc;
    }
    BaseT const& base_alloc() const { return _base_alloc; }

  protected:
    BaseT _base_alloc;
    std::mutex _realloc_lock;
};

template <typename BaseAllocationT> class BaseAllocator : public Allocator<BaseAllocationT, BaseAllocationT> {
  public:
    virtual ~BaseAllocator() = default;
    virtual void write(SubAllocation const& dst_allocation, const void* data, size_t const& byte_size) = 0;
    virtual void get(void* dst, SubAllocation const& src_allocation, size_t const& byte_size) = 0;
};

class CPUAllocator : public BaseAllocator<CPUAllocation> {
  public:
    CPUAllocator(size_t const& byte_size);
    ~CPUAllocator();
    void copy(SubAllocation const& dst_allocation, SubAllocation const& src_allocation, size_t const& byte_size);
    void write(SubAllocation const& dst_allocation, const void* data, size_t const& byte_size);
    void get(void* dst, SubAllocation const& src_allocation, size_t const& byte_size);

    CPUAllocation alloc(size_t const& byte_size);
    void free(CPUAllocation const& allocation);

  private:
    void copy(CPUAllocation const& dst_allocation, CPUAllocation const& src_allocation, size_t const& byte_size);
};

class GPUAllocator : public BaseAllocator<GPUAllocation> {
  public:
    GPUAllocator(VkDeviceSize byte_size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, std::string name);
    ~GPUAllocator();
    void copy(SubAllocation const& dst_allocation, SubAllocation const& src_allocation, size_t const& byte_size);
    void write(SubAllocation const& allocation, const void* data, size_t const& byte_size);
    void get(void* dst, SubAllocation const& src_allocation, size_t const& byte_size);

    GPUAllocation alloc(size_t const& byte_size);
    void free(GPUAllocation const& allocation);

    VkDescriptorDataEXT const& descriptor_data() const { return _descriptor_data.value(); }
    VkDescriptorAddressInfoEXT const& addr_info() const { return _addr_info; }

  private:
    void copy(GPUAllocation const& dst_allocation, GPUAllocation const& src_allocation, size_t const& byte_size);

  private:
    VkBufferCreateInfo _buffer_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    VmaAllocationCreateInfo _alloc_info{};
    std::string _name;
    std::optional<VkDescriptorDataEXT> _descriptor_data;
    VkDescriptorAddressInfoEXT _addr_info{VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT};
};

#endif // BASE_ALLOCATOR_H_
