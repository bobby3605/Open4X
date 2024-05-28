#ifndef RENDERER_H_
#define RENDERER_H_

#include "../Allocator/allocator.hpp"
#include "common.hpp"
#include "rendergraph.hpp"
#include <vulkan/vulkan_core.h>

class Renderer {
  public:
    Renderer(NewSettings* settings);
    ~Renderer();
    void create_data_buffers();
    // Return true if swapchain was recreated
    bool render();
    GPUAllocator* instance_data_allocator;
    StackAllocator<GPUAllocator>* instances_stack_allocator;

  private:
    NewSettings* _settings;
    SwapChain* _swap_chain;
    VkCommandPool _command_pool;
    void recreate_swap_chain();

    void create_rendergraph();
    RenderGraph* rg;

    // FIXME:
    // set these dynamically
    VkViewport viewport{};
    VkRect2D scissor{};
};

#endif // RENDERER_H_
