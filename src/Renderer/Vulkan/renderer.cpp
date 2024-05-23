#include "renderer.hpp"
#include "device.hpp"
#include "memory_manager.hpp"
#include "model.hpp"
#include "pipeline.hpp"
#include "shader.hpp"
#include "swapchain.hpp"
#include "window.hpp"
#include <vulkan/vulkan_core.h>

Renderer::Renderer(NewSettings* settings) : _settings(settings) {
    new Device();
    new MemoryManager();
    create_data_buffers();
    _swap_chain = new SwapChain(Window::window->extent());
    // TODO
    // Probably don't need this anymore
    create_command_buffers();

    create_rendergraph();
}

Renderer::~Renderer() {
    delete _pipeline;
    delete rg;
    delete _swap_chain;
    delete MemoryManager::memory_manager;
    delete Device::device;
}

void Renderer::create_command_buffers() {
    _command_buffers.reserve(SwapChain::MAX_FRAMES_IN_FLIGHT);
    _command_pool = Device::device->command_pools()->get_pool();
    for (uint32_t i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; ++i) {
        _command_buffers.push_back(Device::device->command_pools()->get_buffer(_command_pool));
    }
}

void Renderer::create_data_buffers() {
    Buffer* vertex_buffer = MemoryManager::memory_manager->create_buffer(
        "vertex_buffer", sizeof(NewVertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    Buffer* index_buffer = MemoryManager::memory_manager->create_buffer(
        "index_buffer", sizeof(uint32_t), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    Buffer* instance_data_buffer = MemoryManager::memory_manager->create_buffer(
        "InstanceData", sizeof(InstanceData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
}

void Renderer::recreate_swap_chain() {
    // pause on minimization
    if (_settings->pauseOnMinimization) {
        int width = 0, height = 0;
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(Window::window->glfw_window(), &width, &height);
            glfwWaitEvents();
        }
    }

    vkDeviceWaitIdle(Device::device->vk_device());

    // FIXME:
    // recreate swap chain
    //    swapChain = new VulkanSwapChain(_device, _window->getExtent(), swapChain);
}

bool Renderer::render() {
    // Start Frame
    VkResult result = _swap_chain->acquire_next_image();
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        // TODO
        // Do I need to run acquireNextImage after this?
        recreate_swap_chain();
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image");
    }
    // Submit Frame
    result = _swap_chain->submit_command_buffers(rg->command_buffer());
    // End Frame
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        recreate_swap_chain();
        // return true if framebuffer was resized
        return true;
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image");
    }
    return false;
}

struct CustomDrawCommand {
    VkDrawIndexedIndirectCommand vk_command;
    VkDeviceAddress vertex_address;
    VkDeviceAddress index_address;
};

void Renderer::draw_indirect_custom() {
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

    vkCmdDrawIndexedIndirectCount(_command_buffers[_swap_chain->current_frame()],
                                  MemoryManager::memory_manager->get_buffer("custom_indirect_commands")->vk_buffer(), 0,
                                  MemoryManager::memory_manager->get_buffer("custom_indirect_count")->vk_buffer(), 0,
                                  custom_draw_commands.size(), sizeof(CustomDrawCommand));
}

/*
void Renderer::record_render_ops(std::vector<RenderOp>& render_ops, VkCommandBuffer commandBuffer) {
    for (RenderOp render_op : render_ops) {
        render_op(commandBuffer);
    }
}


RenderOp startRendering(SwapChain* swap_chain) {
    return [&](VkCommandBuffer command_buffer) {
        VkRenderingAttachmentInfo color_attachment{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
        color_attachment.imageView = swap_chain->color_image_view();
        color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment.clearValue.color = {0.0f, 0.0f, 0.0f, 1.0f};

        VkRenderingAttachmentInfo depth_attachment{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
        depth_attachment.imageView = swap_chain->depth_image_view();
        depth_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment.clearValue.depthStencil = {1.0f, 0};

        VkRenderingInfo pass_info{VK_STRUCTURE_TYPE_RENDERING_INFO};
        pass_info.renderArea.extent = swap_chain->extent();
        pass_info.layerCount = 1;
        pass_info.colorAttachmentCount = 1;
        pass_info.pColorAttachments = &color_attachment;
        pass_info.pDepthAttachment = &depth_attachment;

        Device::device->transition_image_layout(swap_chain->color_image_view(), VK_IMAGE_LAYOUT_UNDEFINED,
                                                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                                VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1})(command_buffer);

        Device::device->transitionImageLayout(swap_chain->depth_image_view(), VK_IMAGE_LAYOUT_UNDEFINED,
                                              VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                              VkImageSubresourceRange{VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1})(command_buffer);

        vkCmdBeginRendering(command_buffer, &pass_info);
    };
}

RenderOp endRendering(SwapChain* swap_chain) {
    return [=](VkCommandBuffer command_buffer) {
        vkCmdEndRendering(command_buffer);

        Device::device->transitionImageLayout(swap_chain->color_image_view(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                              VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                              VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1})(command_buffer);
    };
}

RenderOp pushConstants(std::shared_ptr<VulkanShader> shader, void* data) {
    return [=](VkCommand_buffer command_buffer) {
        vkCmdPushConstants(command_buffer, pipelines[getFilenameNoExt(shader->name)]->pipelineLayout(), shader->stageFlags,
                           shader->pushConstantRange.offset, shader->pushConstantRange.size, data);
    };
}

RenderOp fill_buffer(std::string buffer_name, VkDeviceSize offset, VkDeviceSize size, uint32_t value) {
    return [=](VkCommandBuffer command_buffer) {
        vkCmdFillBuffer(command_buffer, MemoryManager::memory_manager->get_buffer(buffer_name)->vk_buffer(), offset, size, value);
    };
}

RenderOp dispatch(uint32_t group_count_X, uint32_t group_count_Y, uint32_t group_count_Z) {
    return [=](VkCommandBuffer command_buffer) { vkCmdDispatch(command_buffer, group_count_X, group_count_Y, group_count_Z); };
}

RenderOp drawIndexedIndirectCount(std::string draws_buffer_name, VkDeviceSize offset, std::string count_buffer_name,
                                  VkDeviceSize count_buffer_offset, uint32_t max_draw_count, uint32_t stride) {
    return [=](VkCommandBuffer command_buffer) {
        vkCmdDrawIndexedIndirectCount(command_buffer, MemoryManager::memory_manager->get_buffer(draws_buffer_name)->vk_buffer(), offset,
                                      MemoryManager::memory_manager->get_buffer(count_buffer_name)->vk_buffer(), count_buffer_offset,
                                      max_draw_count, stride);
    };
}

RenderOp write_timestamp(VkPipelineStageFlags2 stage_flags, VkQueryPool query_pool, uint32_t query) {
    return [=](VkCommandBuffer command_buffer) { vkCmdWriteTimestamp2(command_buffer, stage_flags, query_pool, query); };
}

RenderOp reset_query_pool(VkQueryPool query_pool, uint32_t first_query, uint32_t count) {
    return [=](VkCommandBuffer command_buffer) { vkCmdResetQueryPool(command_buffer, query_pool, first_query, count); };
}

RenderOp bind_vertex_buffer(std::string vertex_buffer_name) {
    return [=](VkCommandBuffer command_buffer) {
        // TODO
        // There might be an issue here if the vertex buffer gets resized before this command finishes executing
        VkBuffer vertex_buffers[] = {MemoryManager::memory_manager->get_buffer(vertex_buffer_name)->vk_buffer()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
    };
}

RenderOp bind_index_buffer(std::string index_buffer_name) {
    return [=](VkCommandBuffer command_buffer) {
        vkCmdBindIndexBuffer(command_buffer, MemoryManager::memory_manager->get_buffer(index_buffer_name)->vk_buffer(), 0,
                             VK_INDEX_TYPE_UINT32);
    };
}

RenderOp memory_barrier(VkAccessFlags2 src_access_mask, VkPipelineStageFlags2 src_stage_mask, VkAccessFlags2 dst_access_mask,
                        VkPipelineStageFlags2 dst_stage_mask) {
    return [=](VkCommandBuffer command_buffer) {
        VkMemoryBarrier2 memory_barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER_2};
        memory_barrier.pNext = VK_NULL_HANDLE;
        memory_barrier.srcAccessMask = src_access_mask;
        memory_barrier.srcStageMask = src_stage_mask;
        memory_barrier.dstAccessMask = dst_access_mask;
        memory_barrier.dstStageMask = dst_stage_mask;

        VkDependencyInfo dependency_info{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
        dependency_info.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        dependency_info.memoryBarrierCount = 1;
        dependency_info.pMemoryBarriers = &memory_barrier;

        vkCmdPipelineBarrier2(command_buffer, &dependency_info);
    };
}

RenderOp setViewport(VkViewport viewport) {
    return [=](VkCommandBuffer command_buffer) { vkCmdSetViewport(command_buffer, 0, 1, &viewport); };
}

RenderOp setScissor(VkRect2D scissor) {
    return [=](VkCommandBuffer command_buffer) { vkCmdSetScissor(command_buffer, 0, 1, &scissor); };
}
*/

void Renderer::create_rendergraph() {
    rg = new RenderGraph(_command_pool);

    rg->begin_rendering(_swap_chain);
    std::string baseShaderPath = "assets/shaders/graphics/";
    std::string baseShaderName = "triangle";

    std::string vert_shader_path = baseShaderPath + baseShaderName + ".vert";
    std::string frag_shader_path = baseShaderPath + baseShaderName + ".frag";
    rg->graphics_pass(vert_shader_path, frag_shader_path, "vertex_buffer", "index_buffer", _swap_chain);

    std::vector<VkDrawIndexedIndirectCommand> indirect_draws;

    rg->add_node(vkCmdDrawIndexedIndirectCount, MemoryManager::memory_manager->get_buffer("indirect_commands")->vk_buffer(), 0,
                 MemoryManager::memory_manager->get_buffer("indirect_count")->vk_buffer(), 0, indirect_draws.size(),
                 sizeof(indirect_draws[0]));

    rg->add_node(vkCmdEndRendering);
    rg->compile();
}
