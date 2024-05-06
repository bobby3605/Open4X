#include "rendergraph.hpp"
#include "device.hpp"
#include "memory_manager.hpp"
#include "model.hpp"
#include "swapchain.hpp"
#include "window.hpp"
#include <vulkan/vulkan_core.h>

RenderGraph::RenderGraph() {
    new Device();
    new MemoryManager();
    create_data_buffers();
    _swap_chain = new SwapChain(Window::window->extent());
    create_command_buffers();
}

RenderGraph::~RenderGraph() {
    delete _swap_chain;
    delete MemoryManager::memory_manager;
    delete Device::device;
}

void RenderGraph::create_command_buffers() {
    _command_buffers.reserve(SwapChain::MAX_FRAMES_IN_FLIGHT);
    _command_pool = Device::device->command_pools()->get_pool();
    for (uint32_t i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; ++i) {
        _command_buffers.push_back(Device::device->command_pools()->get_buffer(_command_pool));
    }
}

void RenderGraph::create_data_buffers() {
    Buffer* vertex_buffer = MemoryManager::memory_manager->create_buffer(
        "vertex_buffer", sizeof(NewVertex) * 1, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    Buffer* index_buffer = MemoryManager::memory_manager->create_buffer(
        "index_buffer", sizeof(uint32_t) * 1, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    Buffer* instance_data_buffer = MemoryManager::memory_manager->create_buffer(
        "InstanceData", sizeof(InstanceData) * 1, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    /*
    VmaVirtualAllocation alloc;
    VkDeviceSize offset;
    instance_data_buffer->alloc(16, alloc, offset);

    VmaVirtualAllocation alloc1;
    VkDeviceSize offset1;
    instance_data_buffer->alloc(sizeof(InstanceData) * 3, alloc1, offset1);

    instance_data_buffer->free(alloc);
    */
}

void RenderGraph::render() { draw_indirect_custom(); }

struct CustomDrawCommand {
    VkDrawIndexedIndirectCommand vk_command;
    VkDeviceAddress vertex_address;
    VkDeviceAddress index_address;
};

void RenderGraph::draw_indirect_custom() {
    VkDrawIndexedIndirectCommand draw_command{};
    draw_command.firstIndex = 0;
    draw_command.firstInstance = 0;
    draw_command.indexCount = 0;
    draw_command.instanceCount = 0;
    draw_command.vertexOffset = 0;

    std::vector<CustomDrawCommand> custom_draw_commands;
    CustomDrawCommand custom_draw_command;
    custom_draw_command.vk_command = draw_command;

    VkBufferDeviceAddressInfo vertex_address_info{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
    vertex_address_info.buffer = MemoryManager::memory_manager->get_buffer("vertex_buffer")->vk_buffer();
    custom_draw_command.vertex_address = vkGetBufferDeviceAddress(Device::device->vk_device(), &vertex_address_info);

    VkBufferDeviceAddressInfo index_address_info{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
    index_address_info.buffer = MemoryManager::memory_manager->get_buffer("index_buffer")->vk_buffer();
    custom_draw_command.index_address = vkGetBufferDeviceAddress(Device::device->vk_device(), &index_address_info);

    custom_draw_commands.push_back(custom_draw_command);

    vkCmdDrawIndexedIndirectCount(_command_buffers[_current_frame],
                                  MemoryManager::memory_manager->get_buffer("custom_indirect_commands")->vk_buffer(), 0,
                                  MemoryManager::memory_manager->get_buffer("custom_indirect_count")->vk_buffer(), 0,
                                  custom_draw_commands.size(), sizeof(CustomDrawCommand));
}
