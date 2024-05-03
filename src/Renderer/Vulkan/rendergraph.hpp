#ifndef RENDERGRAPH_H_
#define RENDERGRAPH_H_

#include "swapchain.hpp"
#include <vulkan/vulkan_core.h>

class RenderGraph {
  public:
    RenderGraph();
    ~RenderGraph();

  private:
    SwapChain* _swap_chain;
    VkCommandPool _command_pool;
    std::vector<VkCommandBuffer> _command_buffers;
    void create_command_buffers();
};

#endif // RENDERGRAPH_H_
