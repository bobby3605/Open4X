#ifndef DRAW_H_
#define DRAW_H_

#include "../../utils/math.hpp"
#include "../../utils/utils.hpp"
#include "../Allocator/sub_allocator.hpp"
#include <glm/gtx/hash.hpp>
#include <mutex>
#include <vulkan/vulkan_core.h>

struct DrawAllocators {
    LinearAllocator<GPUAllocation>* vertex;
    LinearAllocator<GPUAllocation>* index;
    ContiguousFixedAllocator<GPUAllocation>* indirect_commands;
    ContiguousFixedAllocator<GPUAllocation>* material_indices;
    FixedAllocator<GPUAllocation>* material_data;
    FixedAllocator<GPUAllocation>* indirect_count;
    SubAllocation<FixedAllocator, GPUAllocation>* indirect_count_alloc;
    FixedAllocator<GPUAllocation>* instance_data;
    LinearAllocator<GPUAllocation>* instance_indices;
};

struct InstanceData {
    alignas(32) glm::mat4 model_matrix;
};

struct NewMaterialData {
    alignas(16) glm::vec4 base_color_factor = {1.0f, 1.0f, 1.0f, 1.0f};
    uint sampler_index = 0;
    uint base_texture_index = 0;
    uint normal_index = 0;
    float normal_scale = 1.0f;
    uint metallic_roughness_index = 0;
    float metallic_factor = 1.0f;
    float roughness_factor = 1.0f;
    uint ao_index = 0;
    float occlusion_strength = 1.0f;
    // TODO:
    // Does this need padding?
    // It didn't need any before the rewrite...
};

struct Vertex {
    glm::vec3 pos = {0, 0, 0};
    glm::vec2 tex_coord = {0, 0};
    glm::vec3 normal = {0, 0, 1};
    bool operator==(const Vertex& other) const { return pos == other.pos && tex_coord == other.tex_coord && normal == other.normal; }
};

namespace std {
template <> struct hash<Vertex> {
    size_t operator()(Vertex const& vertex) const {
        return (hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec2>()(vertex.tex_coord))) ^ (hash<glm::vec2>()(vertex.normal));
    }
};
} // namespace std

typedef SubAllocation<FixedAllocator, GPUAllocation>* InstanceDataAlloc;
typedef SubAllocation<ContiguousFixedAllocator, SubAllocation<LinearAllocator, GPUAllocation>>* InstanceIndexAlloc;

struct InstanceAllocPair {
    InstanceDataAlloc data;
    InstanceIndexAlloc index;
};

class Draw {
  public:
    Draw(DrawAllocators const& draw_allocators, std::vector<Vertex> const& vertices, std::vector<uint32_t> const& indices,
         SubAllocation<FixedAllocator, GPUAllocation>* material_alloc, safe_vector<Draw*>& invalid_draws);
    ~Draw();
    void preallocate(uint32_t count);
    void add_instance(InstanceAllocPair& output);
    void remove_instance(InstanceAllocPair instance);
    void write_instance_data(uint32_t instance_id, InstanceData const& instance_data);
    void write_indirect_command();

  private:
    DrawAllocators _allocators;
    VkDrawIndexedIndirectCommand _indirect_command;
    safe_vector<Draw*>& _invalid_draws;
    bool _registered = false;
    void register_invalid();
    size_t _instance_count = 0;
    SubAllocation<LinearAllocator, GPUAllocation>* _vertex_alloc;
    SubAllocation<LinearAllocator, GPUAllocation>* _index_alloc;
    SubAllocation<ContiguousFixedAllocator, GPUAllocation>* _indirect_commands_alloc;
    SubAllocation<ContiguousFixedAllocator, GPUAllocation>* _material_index_alloc;
    ContiguousFixedAllocator<SubAllocation<LinearAllocator, GPUAllocation>>* instance_indices_allocator;
    std::mutex _alloc_lock;
};

#endif // DRAW_H_
