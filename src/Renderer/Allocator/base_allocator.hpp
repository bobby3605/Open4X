#ifndef BASE_ALLOCATOR_H_
#define BASE_ALLOCATOR_H_
#include "allocation.hpp"
#include "vk_mem_alloc.h"
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vulkan/vulkan_core.h>

template <typename AllocationT, typename... Args> class BaseAllocator {
    std::unordered_map<std::string, AllocationT*> _allocations;

  public:
    ~BaseAllocator() {
        for (auto& allocation : _allocations) {
            delete allocation.second;
        }
    }
    AllocationT* create_buffer(Args... args, std::string const& name) {
        AllocationT* allocation;
        if constexpr (std::is_same_v<AllocationT, GPUAllocation>) {
            allocation = new AllocationT(args..., name);
        } else {
            allocation = new AllocationT(args...);
        }
        if (_allocations.count(name) != 0) {
            throw std::runtime_error(name + " already exists!");
        } else {
            _allocations.insert({name, allocation});
        }
        return allocation;
    }
    void free_buffer(std::string name) {
        if (_allocations.count(name) == 1) {
            delete _allocations.at(name);
            _allocations.erase(name);
        } else {
            throw std::runtime_error("error: buffer double free: " + name);
        }
    }
    AllocationT* get_buffer(std::string const& name) {
        if (_allocations.count(name) == 1) {
            return _allocations.at(name);
        } else {
            throw std::runtime_error("error: buffer not found: " + name);
        }
    }
    bool buffer_exists(std::string const& name) { return _allocations.contains(name); }
};

class CPUAllocator : public BaseAllocator<CPUAllocation> {
  public:
    CPUAllocator();
};
inline CPUAllocator* cpu_allocator;

class GPUAllocator : public BaseAllocator<GPUAllocation, VkBufferUsageFlags, VkMemoryPropertyFlags> {
  public:
    GPUAllocator();
};
inline GPUAllocator* gpu_allocator;

#endif // BASE_ALLOCATOR_H_
