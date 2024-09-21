#include "rendergraph.hpp"
#include "../Allocator/base_allocator.hpp"
#include "command_runner.hpp"
#include "common.hpp"
#include "device.hpp"
#include "pipeline.hpp"
#include "swapchain.hpp"
#include "window.hpp"
#include <cstdint>
#include <memory>
#include <vulkan/vulkan_core.h>

RenderGraph::RenderGraph(VkCommandPool pool) : _pool{pool} {

    _swap_chain = new SwapChain(Window::window->extent());
    if (Device::device->use_descriptor_buffers()) {
        _descriptor_buffer_allocator = new LinearAllocator<GPUAllocation>(gpu_allocator->create_buffer(
            VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            "_descriptor_buffer_allocator"));
    }
    for (uint32_t i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; ++i) {
        _command_buffers.emplace_back(Device::device->command_pools()->get_primary(_pool));
        Device::device->set_debug_name(VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)_command_buffers.back(),
                                       "command_buffers[" + std::to_string(i) + "]");
    }
}

RenderGraph::~RenderGraph() {
    delete _swap_chain;
    for (auto& command_buffer : _command_buffers) {
        Device::device->command_pools()->release_buffer(_pool, command_buffer);
    }
    if (Device::device->use_descriptor_buffers()) {
        // TODO:
        // delete _descriptor_buffer_allocator
        gpu_allocator->free_buffer("_descriptor_buffer_allocator");
    }
}

void RenderGraph::record_buffer(VkCommandBuffer command_buffer, VkCommandBufferUsageFlags flags,
                                std::vector<RenderNode> const& render_nodes) {
    check_result(vkResetCommandBuffer(command_buffer, 0), "failed to reset command buffer!");
    VkCommandBufferBeginInfo begin_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    // Simultaneous use so that it can be used across multiple frames
    // Only works within the same queue family
    // Might need to record per frame, because of secondaries
    // TODO
    // If device doesn't have a queue with both graphics and compute capability,
    // then create separate command buffers for graphics and compute
    begin_info.flags = flags;
    check_result(vkBeginCommandBuffer(command_buffer, &begin_info), "failed to begin command buffer!");
    for (RenderNode const& render_node : render_nodes) {
        render_node.update();
        render_node.op(command_buffer);
    }
    check_result(vkEndCommandBuffer(command_buffer), "failed to end command buffer!");
}

void RenderGraph::recreate_swap_chain() {
    // pause on minimization
    if (settings->pause_on_minimization) {
        int width = 0, height = 0;
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(Window::window->glfw_window(), &width, &height);
            glfwWaitEvents();
        }
    }

    vkDeviceWaitIdle(Device::device->vk_device());

    delete _swap_chain;
    _swap_chain = new SwapChain(Window::window->extent());
}

bool RenderGraph::render() {
    bool recreated = false;
    // Start Frame
    VkResult result = _swap_chain->acquire_next_image();
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        // TODO
        // Do I need to run acquireNextImage after this?
        recreate_swap_chain();
        recreated = true;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image");
    }
    // Submit Frame
    result = submit();
    // End Frame
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        recreate_swap_chain();
        recreated = true;
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image");
        exit(1);
    }
    return recreated;
}

VkResult RenderGraph::submit() {

    record_buffer(_command_buffers[_swap_chain->current_frame()], VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, _graph);

    return _swap_chain->submit_command_buffer(_command_buffers[_swap_chain->current_frame()]);
}

void RenderGraph::begin_rendering() {

    auto viewport = std::make_shared<VkViewport>();
    viewport->x = 0.0f;
    viewport->minDepth = 0.0f;
    viewport->maxDepth = 1.0f;
    /*
    viewport->y = extent.height;
    viewport->width = extent.width;
    viewport->height = extent.height;
    */

    auto scissor = std::make_shared<VkRect2D>();
    scissor->offset = {0, 0};
    /*
    scissor->extent = extent;
    */

    add_node(
        {viewport},
        [&, viewport]() {
            viewport->width = _swap_chain->extent().width;
            viewport->y = 0;
            viewport->height = _swap_chain->extent().height;
        },
        vkCmdSetViewport, 0, 1, viewport.get());
    add_node(
        {scissor}, [&, scissor]() { scissor->extent = _swap_chain->extent(); }, vkCmdSetScissor, 0, 1, scissor.get());

    // TODO:
    // Better solution for image transitions
    std::shared_ptr<VkImageMemoryBarrier2> tmp_barrier;
    std::shared_ptr<VkDependencyInfo> dependency_info;
    std::tie(tmp_barrier, dependency_info) =
        CommandRunner::transition_image(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                        VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});
    add_node(
        {tmp_barrier, dependency_info}, [&, tmp_barrier, dependency_info]() { tmp_barrier->image = _swap_chain->current_image(); },
        vkCmdPipelineBarrier2, dependency_info.get());

    std::shared_ptr<VkImageMemoryBarrier2> tmp_barrier_2;
    std::shared_ptr<VkDependencyInfo> dependency_info_2;
    std::tie(tmp_barrier_2, dependency_info_2) =
        CommandRunner::transition_image(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                        VkImageSubresourceRange{VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1});
    add_node(
        {tmp_barrier_2, dependency_info_2}, [&, tmp_barrier_2, dependency_info_2]() { tmp_barrier_2->image = _swap_chain->depth_image(); },
        vkCmdPipelineBarrier2, dependency_info_2.get());

    auto color_attachment = std::make_shared<VkRenderingAttachmentInfo>();
    color_attachment->sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    // NOTE:
    // Set in update()
    //    color_attachment->imageView = swap_chain->current_image_view();
    color_attachment->imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attachment->loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment->clearValue.color = {0.0f, 0.0f, 0.0f, 1.0f};

    auto depth_attachment = std::make_shared<VkRenderingAttachmentInfo>();
    depth_attachment->sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    // NOTE:
    // Set in update()
    // depth_attachment->imageView = swap_chain->depth_image_view();
    depth_attachment->imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depth_attachment->loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment->storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment->clearValue.depthStencil = {1.0f, 0};

    auto pass_info = std::make_shared<VkRenderingInfo>();
    pass_info->sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    // NOTE:
    // Set in update()
    // pass_info->renderArea.extent = swap_chain->extent();
    pass_info->layerCount = 1;
    pass_info->colorAttachmentCount = 1;
    pass_info->pColorAttachments = color_attachment.get();
    pass_info->pDepthAttachment = depth_attachment.get();

    add_node(
        {color_attachment, depth_attachment, pass_info},
        [&, color_attachment, depth_attachment, pass_info]() {
            color_attachment->imageView = _swap_chain->current_image_view();
            depth_attachment->imageView = _swap_chain->depth_image_view();
            pass_info->renderArea.extent = _swap_chain->extent();
        },
        vkCmdBeginRendering, pass_info.get());

    if (Device::device->use_descriptor_buffers()) {

        // bind descriptor buffer
        auto descriptor_buffers_binding_info = std::make_shared<VkDescriptorBufferBindingInfoEXT>();
        descriptor_buffers_binding_info->sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT;
        descriptor_buffers_binding_info->address = _descriptor_buffer_allocator->parent()->addr_info().address;
        descriptor_buffers_binding_info->usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;

        add_node(
            {descriptor_buffers_binding_info},
            [&, descriptor_buffers_binding_info]() {
                descriptor_buffers_binding_info->address = _descriptor_buffer_allocator->parent()->addr_info().address;
            },
            vkCmdBindDescriptorBuffersEXT_, 1, descriptor_buffers_binding_info.get());
    }
}
void RenderGraph::end_rendering() {
    add_node(vkCmdEndRendering);

    std::shared_ptr<VkImageMemoryBarrier2> tmp_barrier;
    std::shared_ptr<VkDependencyInfo> dependency_info;
    std::tie(tmp_barrier, dependency_info) =
        CommandRunner::transition_image(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                        VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});
    add_node(
        {tmp_barrier, dependency_info}, [&, tmp_barrier, dependency_info]() { tmp_barrier->image = _swap_chain->current_image(); },
        vkCmdPipelineBarrier2, dependency_info.get());
}

void RenderGraph::graphics_pass(std::filesystem::path const& vert_path, std::filesystem::path const& frag_path,
                                LinearAllocator<GPUAllocation>* vertex_buffer_allocator,
                                LinearAllocator<GPUAllocation>* index_buffer_allocator, void* vert_spec_data, void* frag_spec_data,
                                VkPrimitiveTopology const& topology) {

    VkPipelineRenderingCreateInfo pipeline_rendering_info{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
    pipeline_rendering_info.colorAttachmentCount = 1;
    pipeline_rendering_info.pColorAttachmentFormats = &_swap_chain->surface_format().format;
    pipeline_rendering_info.depthAttachmentFormat = _swap_chain->depth_format();

    std::shared_ptr<GraphicsPipeline> pipeline;
    if (Device::device->use_descriptor_buffers()) {
        pipeline = std::make_shared<GraphicsPipeline>(pipeline_rendering_info, _swap_chain->extent(), vert_path, frag_path, vert_spec_data,
                                                      frag_spec_data, topology, _descriptor_buffer_allocator);
    } else {
        pipeline = std::make_shared<GraphicsPipeline>(pipeline_rendering_info, _swap_chain->extent(), vert_path, frag_path, vert_spec_data,
                                                      frag_spec_data, topology);
    }

    add_pipeline(pipeline);

    // set vertex and index buffers
    auto offsets = std::make_shared<std::vector<VkDeviceSize>>(1, 0);
    add_dynamic_node({offsets}, [&, offsets, vertex_buffer_allocator](RenderNode& node) {
        node.op = [&, offsets, vertex_buffer_allocator](VkCommandBuffer cmd_buffer) {
            vkCmdBindVertexBuffers(cmd_buffer, 0, 1, &vertex_buffer_allocator->parent()->buffer(), offsets->data());
        };
    });
    add_dynamic_node({}, [&, index_buffer_allocator](RenderNode& node) {
        node.op = [&, index_buffer_allocator](VkCommandBuffer cmd_buffer) {
            vkCmdBindIndexBuffer(cmd_buffer, index_buffer_allocator->parent()->buffer(), 0, VK_INDEX_TYPE_UINT32);
        };
    });
}

void RenderGraph::compute_pass(std::filesystem::path const& compute_path, void* compute_spec_data) {
    std::shared_ptr<ComputePipeline> pipeline;
    if (Device::device->use_descriptor_buffers()) {
        pipeline = std::make_shared<ComputePipeline>(compute_path, compute_spec_data, _descriptor_buffer_allocator);
    } else {
        pipeline = std::make_shared<ComputePipeline>(compute_path, compute_spec_data);
    }
    add_pipeline(pipeline);
}

void RenderGraph::memory_barrier(VkAccessFlags2 src_access_mask, VkPipelineStageFlags2 src_stage_mask, VkAccessFlags2 dst_access_mask,
                                 VkPipelineStageFlags2 dst_stage_mask) {
    auto memory_barrier = std::make_shared<VkMemoryBarrier2>();
    memory_barrier->sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
    memory_barrier->pNext = VK_NULL_HANDLE;
    memory_barrier->srcAccessMask = src_access_mask;
    memory_barrier->srcStageMask = src_stage_mask;
    memory_barrier->dstAccessMask = dst_access_mask;
    memory_barrier->dstStageMask = dst_stage_mask;

    auto dependency_info = std::make_shared<VkDependencyInfo>();
    dependency_info->sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependency_info->dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    dependency_info->memoryBarrierCount = 1;
    dependency_info->pMemoryBarriers = memory_barrier.get();

    add_node({memory_barrier, dependency_info}, void_update, vkCmdPipelineBarrier2, dependency_info.get());
}

/*
void RenderGraph::fill_buffer(std::string buffer_name, VkDeviceSize offset, VkDeviceSize size, uint32_t value) {
    return [=](VkCommandBuffer command_buffer) {
        vkCmdFillBuffer(command_buffer, MemoryManager::memory_manager->get_buffer(buffer_name)->vk_buffer(), offset, size, value);
    };
}
*/
