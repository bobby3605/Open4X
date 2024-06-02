#ifndef RENDERER_H_
#define RENDERER_H_

#include "common.hpp"
#include "draw.hpp"
#include "rendergraph.hpp"
#include <vulkan/vulkan_core.h>

class Renderer {
  public:
    Renderer(NewSettings* settings);
    ~Renderer();
    void create_data_buffers();
    // Return true if swapchain was recreated
    bool render();
    DrawAllocators draw_allocators;

  private:
    NewSettings* _settings;
    VkCommandPool _command_pool;

    void create_rendergraph();
    RenderGraph* rg;
};

#endif // RENDERER_H_
