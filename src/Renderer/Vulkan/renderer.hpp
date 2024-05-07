#ifndef RENDERER_H_
#define RENDERER_H_

#include "common.hpp"
#include "model.hpp"
#include "swapchain.hpp"
#include <vulkan/vulkan_core.h>

class Renderer {
  public:
    Renderer(NewSettings* settings);
    ~Renderer();
    void create_data_buffers();
    // Return true if swapchain was recreated
    bool render();

  private:
    NewSettings* _settings;
    SwapChain* _swap_chain;
    VkCommandPool _command_pool;
    std::vector<VkCommandBuffer> _command_buffers;
    VkCommandBuffer get_current_command_buffer() { return _command_buffers[_swap_chain->current_frame()]; }
    void create_command_buffers();
    void draw_indirect_custom();
    void recreate_swap_chain();
    void start_frame();
    bool end_frame();

    /*
std::vector<RenderOp> _render_ops;
void record_render_ops(std::vector<RenderOp>& render_ops, VkCommandBuffer command_buffer);
*/
};

#endif // RENDERER_H_
