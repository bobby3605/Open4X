#include "draw.hpp"
#include <cstdint>
#include <glm/gtx/string_cast.hpp>
#include <vulkan/vulkan_core.h>

Draw::Draw(DrawAllocators const& draw_allocators, std::vector<NewVertex> const& vertices, std::vector<uint32_t> const& indices,
           SubAllocation material_alloc)
    : _allocators(draw_allocators), _instance_indices(sizeof(uint32_t), _allocators.instance_indices),
      _instance_indices_allocs(sizeof(SubAllocation), new CPUAllocator(sizeof(SubAllocation))) {

    _vertex_alloc = _allocators.vertex->alloc(vertices.size() * sizeof(vertices[0]));
    _indirect_command.vertexOffset = _vertex_alloc.offset / sizeof(vertices[0]);
    _allocators.vertex->write(_vertex_alloc, vertices.data(), _vertex_alloc.size);

    _indirect_command.indexCount = indices.size();
    _index_alloc = _allocators.index->alloc(_indirect_command.indexCount * sizeof(indices[0]));
    _indirect_command.firstIndex = _index_alloc.offset / sizeof(indices[0]);
    _allocators.index->write(_index_alloc, indices.data(), _index_alloc.size);

    _indirect_commands_alloc = _allocators.indirect_commands->alloc();
    // TODO:
    // thread safety
    uint32_t new_size;
    _allocators.indirect_count->get(&new_size, _allocators.indirect_count_alloc, sizeof(uint32_t));
    ++new_size;
    _allocators.indirect_count->write(draw_allocators.indirect_count_alloc, &new_size, sizeof(uint32_t));

    _indirect_command.instanceCount = 0;
    _last_instance_index_alloc.offset = 0;

    write_indirect_command();
}

Draw::~Draw() {
    delete _instance_indices_allocs.parent();
    // NOTE:
    // _instance_indices will get cleaned up automatically
    for (auto& instance_data_alloc : _instance_data_allocs) {
        _allocators.instance_data->free(instance_data_alloc.second);
    }
}

uint32_t Draw::add_instance() {
    // TODO
    // preallocate when creating a large amount of instances
    SubAllocation instance_data_alloc = _allocators.instance_data->alloc();
    SubAllocation instance_index_alloc = _instance_indices.alloc();
    SubAllocation instance_index_alloc_alloc = _instance_indices_allocs.alloc();
    // NOTE:
    // Keep track of the instance_index_allocs so that remove can easily change an instance index
    _instance_indices_allocs.write(instance_index_alloc_alloc, &instance_index_alloc, sizeof(instance_index_alloc));
    // NOTE:
    // Keep track of the last instance index allocation for pop and swap in remove_instance
    if (instance_index_alloc.offset > _last_instance_index_alloc.offset) {
        _last_instance_index_alloc = instance_index_alloc;
        _last_instance_index_instance_id = instance_index_alloc_alloc.offset;
    }
    _instance_data_allocs.insert({instance_index_alloc, instance_data_alloc});
    // Write instance data index to instance indices buffer
    _instance_indices.write(instance_index_alloc, &instance_data_alloc.offset, sizeof(instance_data_alloc.offset));
    // NOTE:
    // Always update firstInstance in case of a reallocation
    _indirect_command.firstInstance = _instance_indices.base_alloc().offset / instance_index_alloc.size;
    // FIXME:
    // get material index alloc and ensure its offset is _indirect_command.firstInstance
    // _allocators.material_data->write(_material_index_alloc, _material_alloc.offset, sizeof(uint32_t));
    // possible solution is to reserve firstInstance + 0 for material_index
    // then all of the instance data goes after that
    // putting material index at instance_indices[firstInstance]
    //_material_index_alloc = _instance_indices.alloc();
    // Problem is with culling and generally issues from mixing buffers together
    _indirect_command.instanceCount++;
    write_indirect_command();
    return instance_index_alloc_alloc.offset;
}

void Draw::remove_instance(uint32_t instance_id) {
    SubAllocation instance_index_alloc_alloc;
    instance_index_alloc_alloc.offset = instance_id;
    SubAllocation instance_to_be_removed_alloc;
    _instance_indices_allocs.get(&instance_to_be_removed_alloc, instance_index_alloc_alloc, sizeof(instance_to_be_removed_alloc));
    // Copy last instance index to the instance index being removed
    // This keeps the memory contiguous for the instance indices
    _instance_indices.copy(instance_to_be_removed_alloc, _last_instance_index_alloc, sizeof(_last_instance_index_alloc));
    _instance_indices.free(_last_instance_index_alloc);
    // Update the SubAllocation for _last_instance_index_instance_id, since it was moved to instance_index_alloc
    SubAllocation last_instance_index_alloc_alloc;
    last_instance_index_alloc_alloc.offset = _last_instance_index_instance_id;
    _instance_indices_allocs.write(last_instance_index_alloc_alloc, &instance_id, sizeof(instance_id));
    // NOTE:
    // The pointer to the last allocated instance index needs to be updated
    // This should be correct, because the memory should be contiguous
    _last_instance_index_alloc.offset -= _last_instance_index_alloc.size;
    SubAllocation& instance_data_alloc = _instance_data_allocs.at(instance_to_be_removed_alloc);
    _allocators.instance_data->free(instance_data_alloc);
    _instance_data_allocs.erase(instance_data_alloc);
    --_indirect_command.instanceCount;
    write_indirect_command();
}

void Draw::write_indirect_command() {
    _allocators.indirect_commands->write(_indirect_commands_alloc, &_indirect_command, _indirect_commands_alloc.size);
}

void Draw::write_instance_data(uint32_t instance_id, InstanceData const& instance_data) {
    SubAllocation instance_indices_allocs_alloc;
    instance_indices_allocs_alloc.offset = instance_id;
    SubAllocation instance_data_alloc;
    _instance_indices_allocs.get(&instance_data_alloc, instance_indices_allocs_alloc, sizeof(instance_data_alloc));
    _allocators.instance_data->write(instance_data_alloc, &instance_data, sizeof(instance_data));
}
