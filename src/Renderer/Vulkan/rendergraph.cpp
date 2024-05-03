#include "rendergraph.hpp"
#include "buffers.hpp"
#include "device.hpp"
#include "swapchain.hpp"
#include "window.hpp"

RenderGraph::RenderGraph() {
    new Device();
    new Buffers();
    _swap_chain = new SwapChain(Window::window->extent());
    create_command_buffers();
}

RenderGraph::~RenderGraph() { delete Device::device; }

void RenderGraph::create_command_buffers() {
    _command_buffers.reserve(SwapChain::MAX_FRAMES_IN_FLIGHT);
    _command_pool = Device::device->command_pools()->get_pool();
    for (uint32_t i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; ++i) {
        _command_buffers.push_back(Device::device->command_pools()->get_buffer(_command_pool));
    }
}
