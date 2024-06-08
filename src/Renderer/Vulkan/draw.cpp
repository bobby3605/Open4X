#include "draw.hpp"
#include <cstdint>
#include <glm/gtx/string_cast.hpp>
#include <vulkan/vulkan_core.h>

Draw::Draw(DrawAllocators const& draw_allocators, std::vector<NewVertex> const& vertices, std::vector<uint32_t> const& indices,
           SubAllocation<FixedAllocator, GPUAllocation>* material_alloc)
    : _allocators(draw_allocators) {

    _vertex_alloc = _allocators.vertex->alloc(vertices.size() * sizeof(vertices[0]));
    _indirect_command.vertexOffset = _vertex_alloc->offset() / sizeof(vertices[0]);
    _vertex_alloc->write(vertices.data());

    _indirect_command.indexCount = indices.size();
    _index_alloc = _allocators.index->alloc(_indirect_command.indexCount * sizeof(indices[0]));
    _indirect_command.firstIndex = _index_alloc->offset() / sizeof(indices[0]);
    _index_alloc->write(indices.data());

    _indirect_commands_alloc = _allocators.indirect_commands->alloc();
    // Increase indirect draw count
    // TODO:
    // thread safety
    uint32_t new_size;
    _allocators.indirect_count_alloc->get(&new_size);
    ++new_size;
    _allocators.indirect_count_alloc->write(&new_size);
    _indirect_command.instanceCount = 0;
    _indirect_commands_alloc->write(&_indirect_command);

    // TODO:
    // Figure out if _allocators.instance_indices->alloc(0) causes any issues
    instance_indices_allocator = new ContiguousFixedAllocator(sizeof(uint32_t), _allocators.instance_indices->alloc(0));
}

Draw::~Draw() {
    delete instance_indices_allocator;
    // TODO
    // Free instance data
}

InstanceAllocPair Draw::add_instance() {
    // TODO
    // preallocate when creating a large amount of instances
    SubAllocation<FixedAllocator, GPUAllocation>* instance_data_alloc = _allocators.instance_data->alloc();
    SubAllocation<ContiguousFixedAllocator, SubAllocation<LinearAllocator, GPUAllocation>>* instance_index_alloc =
        instance_indices_allocator->alloc();
    ++_indirect_command.instanceCount;
    // Update firstInstance in case instance_indices_allocator->alloc() caused _allocators.instance_indices to realloc
    _indirect_command.firstInstance = instance_indices_allocator->parent()->offset();
    _indirect_commands_alloc->write(&_indirect_command);
    // NOTE:
    // Since instance_data_alloc is directly on a GPUAllocation, offset() is the correct index
    // If it was stacked on top of another allocator, then a global_offset() function would be needed
    instance_index_alloc->write(&instance_data_alloc->offset());
    return {instance_data_alloc, instance_index_alloc};
    // FIXME:
    // get material index alloc and ensure its offset is _indirect_command.firstInstance
    // _allocators.material_data->write(_material_index_alloc, _material_alloc.offset, sizeof(uint32_t));
    // possible solution is to reserve firstInstance + 0 for material_index
    // then all of the instance data goes after that
    // putting material index at instance_indices[firstInstance]
    //_material_index_alloc = _instance_indices.alloc();
    // Problem is with culling and generally issues from mixing buffers together
}

void Draw::remove_instance(InstanceAllocPair instance) {
    SubAllocation<FixedAllocator, GPUAllocation>* instance_data_alloc;
    SubAllocation<ContiguousFixedAllocator, SubAllocation<LinearAllocator, GPUAllocation>>* instance_index_alloc;
    std::tie(instance_data_alloc, instance_index_alloc) = instance;

    _allocators.instance_data->free(instance_data_alloc);
    instance_indices_allocator->pop_and_swap(instance_index_alloc);
    --_indirect_command.instanceCount;
    _indirect_commands_alloc->write(&_indirect_command);
}
