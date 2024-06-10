#include "draw.hpp"
#include <cstdint>
#include <glm/gtx/string_cast.hpp>
#include <mutex>
#include <vulkan/vulkan_core.h>

Draw::Draw(DrawAllocators const& draw_allocators, std::vector<NewVertex> const& vertices, std::vector<uint32_t> const& indices,
           SubAllocation<FixedAllocator, GPUAllocation>* material_alloc)
    : _allocators(draw_allocators) {
    std::unique_lock<std::mutex> lock(_alloc_lock);

    _vertex_alloc = _allocators.vertex->alloc(vertices.size() * sizeof(vertices[0]));
    _indirect_command.vertexOffset = _vertex_alloc->offset() / sizeof(vertices[0]);
    _vertex_alloc->write(vertices.data());

    _indirect_command.indexCount = indices.size();
    _index_alloc = _allocators.index->alloc(_indirect_command.indexCount * sizeof(indices[0]));
    _indirect_command.firstIndex = _index_alloc->offset() / sizeof(indices[0]);
    _index_alloc->write(indices.data());

    _indirect_commands_alloc = _allocators.indirect_commands->alloc();
    // put material index at gl_DrawID
    // material_index is always the same as gl_DrawID,
    // because indirect_commands_alloc and material_index_alloc always get
    // alloced and pop and swapped together,
    // so their offsets should be identical
    uint32_t material_index = ((uint32_t)material_alloc->offset()) / sizeof(NewMaterialData);
    _material_index_alloc = _allocators.material_indices->alloc();
    _material_index_alloc->write(&material_index);

    // Increase indirect draw count
    // TODO:
    // thread safety
    uint32_t new_size;
    _allocators.indirect_count_alloc->get(&new_size);
    ++new_size;
    _allocators.indirect_count_alloc->write(&new_size);
    _indirect_command.instanceCount = 0;
    _indirect_commands_alloc->write(&_indirect_command);

    instance_indices_allocator = new ContiguousFixedAllocator(sizeof(uint32_t), _allocators.instance_indices->alloc_0());
}

Draw::~Draw() {
    _allocators.indirect_commands->pop_and_swap(_indirect_commands_alloc);
    _allocators.material_indices->pop_and_swap(_material_index_alloc);
    delete instance_indices_allocator;
    // TODO
    // Free instance data
}

void Draw::preallocate(uint32_t count) {
    std::unique_lock<std::mutex> lock(_alloc_lock);
    _allocators.instance_data->preallocate(count);
    instance_indices_allocator->preallocate(count);
}

InstanceAllocPair Draw::add_instance() {
    std::unique_lock<std::mutex> lock(_alloc_lock);
    // TODO
    // preallocate when creating a large amount of instances
    SubAllocation<FixedAllocator, GPUAllocation>* instance_data_alloc = _allocators.instance_data->alloc();
    SubAllocation<ContiguousFixedAllocator, SubAllocation<LinearAllocator, GPUAllocation>>* instance_index_alloc =
        instance_indices_allocator->alloc();
    ++_indirect_command.instanceCount;
    // Update firstInstance in case instance_indices_allocator->alloc() caused _allocators.instance_indices to realloc
    _indirect_command.firstInstance = instance_indices_allocator->parent()->offset() / sizeof(uint32_t);
    _indirect_commands_alloc->write(&_indirect_command);
    // NOTE:
    // Since instance_data_alloc is directly on a GPUAllocation, offset() is the correct index
    // If it was stacked on top of another allocator, then a global_offset() function would be needed
    uint32_t instance_data_index = ((uint32_t)instance_data_alloc->offset()) / sizeof(InstanceData);
    instance_index_alloc->write(&instance_data_index);
    return {instance_data_alloc, instance_index_alloc};
}

void Draw::remove_instance(InstanceAllocPair instance) {
    std::unique_lock<std::mutex> lock(_alloc_lock);
    SubAllocation<FixedAllocator, GPUAllocation>* instance_data_alloc;
    SubAllocation<ContiguousFixedAllocator, SubAllocation<LinearAllocator, GPUAllocation>>* instance_index_alloc;
    std::tie(instance_data_alloc, instance_index_alloc) = instance;

    _allocators.instance_data->free(instance_data_alloc);
    instance_indices_allocator->pop_and_swap(instance_index_alloc);
    --_indirect_command.instanceCount;
    _indirect_commands_alloc->write(&_indirect_command);
}
