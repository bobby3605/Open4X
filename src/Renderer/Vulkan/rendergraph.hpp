#ifndef RENDERGRAPH_H_
#define RENDERGRAPH_H_

#include "swapchain.hpp"
#include <vulkan/vulkan_core.h>

class RenderGraph {
  public:
    RenderGraph();
    ~RenderGraph();
    void create_data_buffers();
    void render();

  private:
    SwapChain* _swap_chain;
    VkCommandPool _command_pool;
    std::vector<VkCommandBuffer> _command_buffers;
    void create_command_buffers();
    uint32_t _current_frame;
    void draw_indirect_custom();
};

#endif // RENDERGRAPH_H_
