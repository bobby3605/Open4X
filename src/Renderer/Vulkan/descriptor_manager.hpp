#ifndef DESCRIPTOR_MANAGER_H_
#define DESCRIPTOR_MANAGER_H_

#include "../Allocator/base_allocator.hpp"
#include "pipeline.hpp"
#include <set>
#include <vulkan/vulkan_core.h>

class DescriptorManager {
  public:
    DescriptorManager(GPUAllocator* gpu_allocator);
    void update(std::shared_ptr<Pipeline> pipeline);
    void register_invalid(std::string descriptor_name);
    static DescriptorManager* descriptor_manager;

  private:
    std::set<std::string> _invalid_descriptors;
    GPUAllocator* _gpu_allocator;
};

#endif // DESCRIPTOR_MANAGER_H_
