#include "draw.hpp"
#include <vulkan/vulkan_core.h>

Draw::Draw(Buffer* vertex_buffer, Buffer* index_buffer, Buffer* instance_indices_buffer, Model::Mesh::Primitive* primitive)
    : _vertex_buffer(vertex_buffer), _index_buffer(index_buffer), _instance_indices_buffer(instance_indices_buffer), _primitive(primitive) {

    auto& vertices = _primitive->vertices();
    auto& indices = _primitive->indices();
    _indirect_command.indexCount = indices.capacity();
    // NOTE:
    // Need VkDeviceSize for alloc, so _indirect_command can't be used directly
    VkDeviceSize vertex_offset;
    vertex_buffer->alloc(vertices.capacity(), _vertex_alloc, vertex_offset);
    _indirect_command.vertexOffset = vertex_offset;
    memcpy(vertex_buffer->data(), vertices.data(), vertices.capacity());

    VkDeviceSize first_index;
    index_buffer->alloc(indices.capacity(), _index_alloc, first_index);
    _indirect_command.firstIndex = first_index;
    memcpy(index_buffer->data(), indices.data(), indices.capacity());

    StackAllocator<GPUAllocator>* indirect_commands_allocator;
    SubAllocation indirect_alloc = indirect_commands_allocator->alloc();
    // FIXME:
    // That isn't right
    // indirect_alloc.offset is the location of the indirect command
    // Need to allocate space for the instances
    //    _indirect_command.firstInstance = indirect_alloc.offset;
    _indirect_command.instanceCount = 0;
    // indirect_commands_allocator->write(&_indirect_command, indirect_alloc);
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
    StackAllocator<GPUAllocator>* indirect_commands_allocator;
    SubAllocation indirect_alloc;
    uint32_t instance_id = _indirect_command.instanceCount++;
    //    indirect_commands_allocator->write(&_indirect_command, indirect_alloc);
    return instance_id;
    // Need to allocate instance ids and first instance
    // allocator could keep track of that
    // first instance needs to update if a sub allocation is realloced
}

void Draw::remove_instance(uint32_t instance_index) {
    char* data = _instance_indices_buffer->data();
    // Subtract 1 from instance count
    // Copy last instance index to the instance index being removed
    data[_indirect_command.firstInstance + instance_index] = data[_indirect_command.firstInstance + --_indirect_command.instanceCount];
}
