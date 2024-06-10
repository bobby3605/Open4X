#ifndef DRAW_H_
#define DRAW_H_

#include <mutex>
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "../Allocator/sub_allocator.hpp"
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
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
    glm::mat4 model_matrix;
};

struct NewMaterialData {
    alignas(16) glm::vec4 base_color_factor = {1.0f, 1.0f, 1.0f, 1.0f};
    uint sampler_index = 0;
    uint image_index = 0;
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

struct NewVertex {
    glm::vec3 pos = {0, 0, 0};
    glm::vec2 tex_coord = {0, 0};
    glm::vec3 normal = {0, 0, 1};
    bool operator==(const NewVertex& other) const { return pos == other.pos && tex_coord == other.tex_coord && normal == other.normal; }

    static VkVertexInputBindingDescription binding_description() {
        VkVertexInputBindingDescription binding_description{};
        binding_description.binding = 0;
        binding_description.stride = sizeof(NewVertex);
        binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return binding_description;
    }

    static std::array<VkVertexInputAttributeDescription, 3> attribute_descriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attribute_descriptions;
        attribute_descriptions[0].binding = 0;
        attribute_descriptions[0].location = 0;
        attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attribute_descriptions[0].offset = offsetof(NewVertex, pos);

        attribute_descriptions[1].binding = 0;
        attribute_descriptions[1].location = 1;
        attribute_descriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
        attribute_descriptions[1].offset = offsetof(NewVertex, tex_coord);

        attribute_descriptions[2].binding = 0;
        attribute_descriptions[2].location = 2;
        attribute_descriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        attribute_descriptions[2].offset = offsetof(NewVertex, normal);

        return attribute_descriptions;
    }
};

namespace std {
template <> struct hash<NewVertex> {
    size_t operator()(NewVertex const& vertex) const {
        return (hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec2>()(vertex.tex_coord))) ^ (hash<glm::vec2>()(vertex.normal));
    }
};
} // namespace std

typedef SubAllocation<FixedAllocator, GPUAllocation>* InstanceDataAlloc;
typedef SubAllocation<ContiguousFixedAllocator, SubAllocation<LinearAllocator, GPUAllocation>>* InstanceIndexAlloc;

typedef std::tuple<InstanceDataAlloc, InstanceIndexAlloc> InstanceAllocPair;

class Draw {
  public:
    Draw(DrawAllocators const& draw_allocators, std::vector<NewVertex> const& vertices, std::vector<uint32_t> const& indices,
         SubAllocation<FixedAllocator, GPUAllocation>* material_alloc);
    ~Draw();
    void preallocate(uint32_t count);
    InstanceAllocPair add_instance();
    void remove_instance(InstanceAllocPair instance);
    void write_instance_data(uint32_t instance_id, InstanceData const& instance_data);

  private:
    DrawAllocators _allocators;
    VkDrawIndexedIndirectCommand _indirect_command;
    SubAllocation<LinearAllocator, GPUAllocation>* _vertex_alloc;
    SubAllocation<LinearAllocator, GPUAllocation>* _index_alloc;
    SubAllocation<ContiguousFixedAllocator, GPUAllocation>* _indirect_commands_alloc;
    SubAllocation<ContiguousFixedAllocator, GPUAllocation>* _material_index_alloc;
    ContiguousFixedAllocator<SubAllocation<LinearAllocator, GPUAllocation>>* instance_indices_allocator;
    std::mutex _alloc_lock;
};

#endif // DRAW_H_
