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
            free(allocation.second);
        }
    }
    AllocationT* create_buffer(Args... args, std::string const& name) {
        AllocationT* allocation = new AllocationT(args...);
        if (_allocations.count(name) != 0) {
            throw std::runtime_error(name + " already exists!");
        } else {
            _allocations.insert({name, allocation});
        }
        return allocation;
    }
    void free_buffer(std::string name) {
        if (_allocations.count(name) == 1) {
            free(_allocations.at(name));
            _allocations.erase(name);
        } else {
            throw std::runtime_error("error: buffer double free: " + name);
        }
    }
};

class CPUAllocator : public BaseAllocator<CPUAllocation> {};

class GPUAllocator : public BaseAllocator<GPUAllocation, VkBufferUsageFlags, VkMemoryPropertyFlags> {};

#endif // BASE_ALLOCATOR_H_
