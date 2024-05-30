#ifndef DRAW_H_
#define DRAW_H_

#include <unordered_map>
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "../Allocator/sub_allocator.hpp"
#include <glm/glm.hpp>
#include <set>
#include <vulkan/vulkan_core.h>

struct DrawAllocators {
    LinearAllocator<GPUAllocator>* vertex;
    LinearAllocator<GPUAllocator>* index;
    StackAllocator<GPUAllocator>* indirect_commands;
    StackAllocator<GPUAllocator>* instance_data;
    LinearAllocator<GPUAllocator>* instance_indices;
};

struct InstanceData {
    glm::mat4 model_matrix;
};

class Draw {
  public:
    // NOTE:
    // const void* vertices to allow for any vertex type without templating
    Draw(DrawAllocators const& draw_allocators, const void* vertices, const size_t vertices_size, std::vector<uint32_t> const& indices);
    ~Draw();
    uint32_t add_instance();
    void remove_instance(uint32_t instance_index_alloc);
    void write_instance_data(uint32_t instance_id, InstanceData const& instance_data);

  private:
    DrawAllocators _allocators;
    StackAllocator<LinearAllocator<GPUAllocator>> _instance_indices;
    StackAllocator<CPUAllocator> _instance_indices_allocs;
    SubAllocation _last_instance_index_alloc;
    uint32_t _last_instance_index_instance_id;
    std::unordered_map<SubAllocation, SubAllocation> _instance_data_allocs;
    VkDrawIndexedIndirectCommand _indirect_command;
    SubAllocation _vertex_alloc;
    SubAllocation _index_alloc;
    SubAllocation _indirect_commands_alloc;
    void write_indirect_command();
};

#endif // DRAW_H_
