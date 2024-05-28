#ifndef BASE_ALLOCATOR_H_
#define BASE_ALLOCATOR_H_
#include "../Vulkan/globals.hpp"
#include <cstddef>
#include <mutex>
#include <vulkan/vulkan_core.h>

struct BaseAllocation {
    virtual size_t inline size() = 0;
};

struct SubAllocation : public BaseAllocation {
    size_t offset;
    size_t size_;
    size_t inline size() { return size_; };
};

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
        vmaGetAllocationInfo(globals.device->vma_allocator(), vma_allocation, &info);
        return info.size;
    };
};

template <typename BaseAllocationT> class Allocator {
    static_assert(std::is_base_of<BaseAllocation, BaseAllocationT>::value, "BaseAllocationT must be a BaseAllocation");

  public:
    virtual void copy(SubAllocation const& dst_allocation, SubAllocation const& src_allocation, size_t const& byte_size) = 0;
    virtual void write(SubAllocation const& dst_allocation, const void* data, size_t const& byte_size) = 0;
    BaseAllocationT const& base_alloc() const { return _base_alloc; }

  protected:
    void realloc(BaseAllocationT& allocation, size_t const& byte_size) {
        //    std::lock_guard<std::mutex> lock(_realloc_lock);
        BaseAllocationT new_alloc = alloc(byte_size);
        copy(new_alloc, allocation);
        free(allocation);
        allocation = new_alloc;
    }

    BaseAllocationT _base_alloc;

    virtual BaseAllocationT alloc(size_t const& byte_size) = 0;
    virtual void free(BaseAllocationT const& base_alloc) = 0;
};

template <typename BaseAllocationT> class BaseAllocator : public Allocator<BaseAllocationT> {
  public:
    BaseAllocator(size_t const& byte_size) { Allocator<BaseAllocationT>::_base_alloc = alloc<BaseAllocationT>(byte_size); }
    ~BaseAllocator() { free(Allocator<BaseAllocationT>::_base_alloc); };

  private:
    virtual void copy(BaseAllocationT const& dst_allocation, BaseAllocationT const& src_allocation, size_t const& byte_size) = 0;
};

class CPUAllocator : protected BaseAllocator<CPUAllocation> {
  public:
    CPUAllocator(size_t const& byte_size);
    void copy(SubAllocation const& dst_allocation, SubAllocation const& src_allocation, size_t const& byte_size);
    void write(SubAllocation const& dst_allocation, const void* data, size_t const& byte_size);

  private:
    CPUAllocation alloc(size_t const& byte_size);
    void free(CPUAllocation const& allocation);
    void copy(CPUAllocation const& dst_allocation, CPUAllocation const& src_allocation, size_t const& byte_size);
};

class GPUAllocator : protected BaseAllocator<GPUAllocation> {
  public:
    GPUAllocator(VkDeviceSize byte_size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, std::string name);
    void copy(SubAllocation const& dst_allocation, SubAllocation const& src_allocation, size_t const& byte_size);
    void write(SubAllocation const& allocation, const void* data, size_t const& byte_size);

  private:
    GPUAllocation alloc(size_t const& byte_size);
    void free(GPUAllocation const& allocation);
    void copy(GPUAllocation const& dst_allocation, GPUAllocation const& src_allocation, size_t const& byte_size);

    VkBufferCreateInfo _buffer_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    VmaAllocationCreateInfo _alloc_info;
    std::string _name;
    std::optional<VkDescriptorDataEXT> _descriptor_data;
    VkDescriptorAddressInfoEXT _addr_info{VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT};
};

#endif // BASE_ALLOCATOR_H_
