#ifndef DRAW_H_
#define DRAW_H_

#include "../Allocator/sub_allocator.hpp"
#include <vulkan/vulkan_core.h>

class Draw {
  public:
    Draw(LinearAllocator<GPUAllocator>* vertex_allocator, LinearAllocator<GPUAllocator>* index_allocator,
         StackAllocator<GPUAllocator>* indirect_commands_allocator, LinearAllocator<GPUAllocator>* instance_indices_allocator,
         const void* vertices, const size_t vertices_size, std::vector<uint32_t> const& indices);
    // NOTE:
    // const void* vertices to allow for any vertex type without templating
    uint32_t add_instance();
    void remove_instance(uint32_t instance_index);

  private:
    VkDrawIndexedIndirectCommand _indirect_command;
    LinearAllocator<GPUAllocator>* _vertex_allocator;
    SubAllocation _vertex_alloc;
    LinearAllocator<GPUAllocator>* _index_allocator;
    SubAllocation _index_alloc;
    StackAllocator<GPUAllocator>* _indirect_commands_allocator;
    SubAllocation _indirect_commands_alloc;
    StackAllocator<LinearAllocator<GPUAllocator>> _instance_indices_allocator;
    void update_indirect_command();
};

#endif // DRAW_H_
