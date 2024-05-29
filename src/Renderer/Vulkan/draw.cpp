#include "draw.hpp"
#include <vulkan/vulkan_core.h>

Draw::Draw(LinearAllocator<GPUAllocator>* vertex_allocator, LinearAllocator<GPUAllocator>* index_allocator,
           StackAllocator<GPUAllocator>* indirect_commands_allocator, LinearAllocator<GPUAllocator>* instance_indices_allocator,
           const void* vertices, const size_t vertices_size, std::vector<uint32_t> const& indices)
    : _vertex_allocator(vertex_allocator), _index_allocator(index_allocator), _indirect_commands_allocator(indirect_commands_allocator),
      _instance_indices_allocator(sizeof(uint32_t), instance_indices_allocator) {

    _indirect_command.indexCount = indices.size();
    // NOTE:
    // Need VkDeviceSize for alloc, so _indirect_command can't be used directly
    _vertex_alloc = vertex_allocator->alloc(vertices_size);
    _indirect_command.vertexOffset = _vertex_alloc.offset;
    vertex_allocator->write(_vertex_alloc, vertices, vertices_size);

    _index_alloc = index_allocator->alloc(_indirect_command.indexCount);
    _indirect_command.firstIndex = _index_alloc.offset;
    index_allocator->write(_index_alloc, indices.data(), indices.size());

    _indirect_commands_alloc = indirect_commands_allocator->alloc();

    _indirect_command.instanceCount = 0;
}

// TODO
// After add_instance returns an index
// instance_indices_buffer[instance_index] = instance_data_index
// instance_data_buffer[instance_data_index] = InstanceData
//
// add_instance needs to run in the same order that load_instance_data runs in
// so the whole list of instance indices are at the same position as the instance data vector
//
// model->load_instance_data(,instance_data)
// instance_indices; for node: instance_indices.push_back(add_instance)
//
// for i in instance_indices.size():
//
// instance_data_index = i + object->_instance_data_offset
//
// instance_indices_buffer[instance_indices[i]] = instance_data_index
// instance_data_buffer[instance_data_index] = instance_data[i]

// FIXME:
// When remove_instance deletes an instance and copies the last index to the remove position,
// the instance index that was obtained from add_instance at the end of the buffer is now invalid,
// and needs to be updated to the new index before updating instance_data_buffer
//
// Doing this almost certainly breaks the well-defined order of instance_data_buffer,
// so it can't be just directly uploaded, it has to be corrected for the swap
//
// maybe after the first time, instance_data_index can be read from instance_indices_buffer??

uint32_t Draw::add_instance() {
    /*
    VkDeviceSize new_first_instance;
    // TODO
    // preallocate when creating a large amount of instances
    _instance_indices_buffer->realloc(++_indirect_command.instanceCount, _instance_indices_alloc, new_first_instance);
    _indirect_command.firstInstance = new_first_instance;
    return _indirect_command.instanceCount - 1;
    */
    SubAllocation instance_index_alloc = _instance_indices_allocator.alloc();
    _indirect_command.firstInstance = _instance_indices_allocator.base_alloc().offset;
    uint32_t instance_id = _indirect_command.instanceCount++;
    update_indirect_command();
    return instance_id;
    // Need to allocate instance ids and first instance
    // allocator could keep track of that
    // first instance needs to update if a sub allocation is realloced
}

void Draw::remove_instance(uint32_t instance_index) {
    //    char* data = _instance_indices_buffer->data();
    // Subtract 1 from instance count
    // Copy last instance index to the instance index being removed
    // data[_indirect_command.firstInstance + instance_index] = data[_indirect_command.firstInstance + --_indirect_command.instanceCount];
}

void Draw::update_indirect_command() {
    _indirect_commands_allocator->write(_indirect_commands_alloc, &_indirect_command, _indirect_commands_alloc.size());
}
