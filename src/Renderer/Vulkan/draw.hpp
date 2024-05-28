#ifndef DRAW_H_
#define DRAW_H_

#include "../Allocator/allocator.hpp"
#include "buffer.hpp"
#include "model.hpp"
#include <vulkan/vulkan_core.h>

class Draw {
  public:
    Draw(Buffer* vertex_buffer, Buffer* index_buffer, Buffer* instance_indices_buffer, Model::Mesh::Primitive* primitive);
    uint32_t add_instance();
    void remove_instance(uint32_t instance_index);

  private:
    VkDrawIndexedIndirectCommand _indirect_command;
    Buffer* _vertex_buffer;
    Buffer* _index_buffer;
    Buffer* _instance_indices_buffer;
    Model::Mesh::Primitive* _primitive;
    VmaVirtualAllocation _vertex_alloc = VK_NULL_HANDLE;
    VmaVirtualAllocation _index_alloc = VK_NULL_HANDLE;
    VmaVirtualAllocation _instance_indices_alloc = VK_NULL_HANDLE;
};

#endif // DRAW_H_
