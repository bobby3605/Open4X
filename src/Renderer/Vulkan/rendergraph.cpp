#include "rendergraph.hpp"
#include "device.hpp"
#include "memory_manager.hpp"
#include "swapchain.hpp"
#include <vulkan/vulkan_core.h>

/*
void tmp() {
    VkRenderingInfo pass_info{};
    RenderNode begin_rendering = RenderNode(vkCmdBeginRendering, &pass_info);
    RenderNode dispatch = RenderNode(vkCmdDispatch, 0, 1, 2);
}
*/

RenderGraph::RenderGraph(VkCommandPool pool) : _command_buffer{Device::device->command_pools()->get_buffer(pool)} {}

void RenderGraph::compile() { record(); }

void RenderGraph::record() {
    VkCommandBufferBeginInfo begin_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    // Simultaneous use so that it can be used across multiple frames
    // Only works within the same queue family
    // TODO
    // If device doesn't have a queue with both graphics and compute capability,
    // then create separate command buffers for graphics and compute
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    vkBeginCommandBuffer(_command_buffer, &begin_info);

    for (RenderNode const& render_node : _render_nodes) {
        render_node.op(_command_buffer);
    }
    vkEndCommandBuffer(_command_buffer);
}
