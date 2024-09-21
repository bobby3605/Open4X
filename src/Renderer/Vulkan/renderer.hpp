#ifndef RENDERER_H_
#define RENDERER_H_

#include "common.hpp"
#include "draw.hpp"
#include "rendergraph.hpp"
#include <vulkan/vulkan_core.h>

class Renderer {
  public:
    Renderer(Settings* settings);
    ~Renderer();
    void create_data_buffers();
    // Return true if swapchain was recreated
    bool render();
    DrawAllocators draw_allocators;
    RenderGraph* rg;

  private:
    Settings* _settings;
    VkCommandPool _command_pool;
    ContiguousFixedAllocator<GPUAllocation>* _obb_commands;
    SubAllocation<ContiguousFixedAllocator, GPUAllocation>* _obb_command;

    void create_rendergraph();
};

#endif // RENDERER_H_
