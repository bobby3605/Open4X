#ifndef RENDEROPS_H_
#define RENDEROPS_H_

#include "../../utils.hpp"
#include "buffer.hpp"
#include <functional>
#include <string>
#include <vulkan/vulkan_core.h>

typedef std::function<void(VkCommandBuffer)> RenderOp;

class RenderNode {
  public:
    template <typename F, typename... Args> RenderNode(F f, Args... args) : op{partial(f, args...)} {}
    // op should never change once it's initialized
    const RenderOp op;
};

class RenderGraph {
  public:
    RenderGraph(VkCommandPool pool);
    void compile();
    VkCommandBuffer command_buffer() { return _command_buffer; }

  private:
    void record();
    std::vector<RenderNode> _render_nodes;
    VkCommandBuffer _command_buffer;
};

#endif // RENDEROPS_H_
