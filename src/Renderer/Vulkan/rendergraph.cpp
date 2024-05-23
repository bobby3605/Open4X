#include "rendergraph.hpp"
#include "device.hpp"
#include "memory_manager.hpp"
#include "pipeline.hpp"
#include "swapchain.hpp"
#include <memory>
#include <vulkan/vulkan_core.h>

/*
void tmp() {
    VkRenderingInfo pass_info{};
    RenderNode begin_rendering = RenderNode(vkCmdBeginRendering, &pass_info);
    RenderNode dispatch = RenderNode(vkCmdDispatch, 0, 1, 2);
}
*/

RenderGraph::RenderGraph(VkCommandPool pool) : _command_buffer{Device::device->command_pools()->get_buffer(pool)} {}

void RenderGraph::compile() {
    get_descriptors();
    record();
}

void RenderGraph::get_descriptors() {

    for (auto& pipeline : _pipelines) {
        for (auto const& shader : pipeline->shaders()) {
            // FIXME:
            // use map instead of unordered map so that set layouts are sorted
            for (auto const& set_layout : shader.second.set_layouts()) {
                std::size_t size;
                std::size_t byte_offset = 0;
                VkDescriptorDataEXT data;
                VkDescriptorGetInfoEXT descriptor_info{VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT};

                descriptor_info.type;
                descriptor_info.data = data;

                size = Device::device->descriptor_buffer_properties().storageBufferDescriptorSize;

                vkGetDescriptorEXT(
                    Device::device->vk_device(), &descriptor_info, size,
                    MemoryManager::memory_manager->get_buffer(pipeline->name() + std::string(Pipeline::_descriptor_buffer_suffix))->data() +
                        byte_offset);
                byte_offset += size;
            }
        }
    }
}

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

// FIXME:
// Handle resized or re-created swapchain
// Will need to re-record buffer,
// maybe put vkCmdBeginRendering into a secondary buffer? so that the whole thing doesn't need to be re-recorded
void RenderGraph::begin_rendering(SwapChain* const swap_chain) {
    transition_image(swap_chain->color_image(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                     VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

    transition_image(swap_chain->depth_image(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                     VkImageSubresourceRange{VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1});

    auto color_attachment = std::make_shared<VkRenderingAttachmentInfo>(VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO);
    color_attachment->imageView = swap_chain->color_image_view();
    color_attachment->imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attachment->loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment->clearValue.color = {0.0f, 0.0f, 0.0f, 1.0f};

    auto depth_attachment = std::make_shared<VkRenderingAttachmentInfo>(VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO);
    depth_attachment->imageView = swap_chain->depth_image_view();
    depth_attachment->imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depth_attachment->loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment->storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment->clearValue.depthStencil = {1.0f, 0};

    auto pass_info = std::make_shared<VkRenderingInfo>(VK_STRUCTURE_TYPE_RENDERING_INFO);
    pass_info->renderArea.extent = swap_chain->extent();
    pass_info->layerCount = 1;
    pass_info->colorAttachmentCount = 1;
    pass_info->pColorAttachments = color_attachment.get();
    pass_info->pDepthAttachment = depth_attachment.get();

    add_node({color_attachment, depth_attachment, pass_info}, vkCmdBeginRendering, pass_info.get());
}

void RenderGraph::transition_image(VkImage image, VkImageLayout old_layout, VkImageLayout new_layout, uint32_t mip_levels) {
    transition_image(image, old_layout, new_layout, VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, mip_levels, 0, 1});
}
void RenderGraph::transition_image(VkImage image, VkImageLayout old_layout, VkImageLayout new_layout,
                                   VkImageSubresourceRange subresource_range) {
    VkImageMemoryBarrier2 barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;

    barrier.subresourceRange = subresource_range;

    image_barrier(barrier);
}

void RenderGraph::image_barrier(VkImageMemoryBarrier2& barrier) {
    VkImageLayout& old_layout = barrier.oldLayout;
    VkImageLayout& new_layout = barrier.newLayout;

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_2_NONE;
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

        barrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

    } else if (old_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
        barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

        barrier.dstAccessMask = VK_ACCESS_2_NONE;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;

        barrier.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_2_NONE;
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;

        barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;

        barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    } else {
        throw std::invalid_argument("unsupported layout transition");
    }

    auto tmp_barrier = std::make_shared<VkImageMemoryBarrier2>(barrier);
    auto dependency_info = std::make_shared<VkDependencyInfo>(VK_STRUCTURE_TYPE_DEPENDENCY_INFO);

    dependency_info->dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    dependency_info->imageMemoryBarrierCount = 1;
    dependency_info->pImageMemoryBarriers = tmp_barrier.get();

    add_node({tmp_barrier, dependency_info}, vkCmdPipelineBarrier2, dependency_info.get());
}

void RenderGraph::graphics_pass(std::string const& vert_path, std::string const& frag_path, std::string const& vertex_buffer_name,
                                std::string const& index_buffer_name, SwapChain* swap_chain_defaults) {

    VkPipelineRenderingCreateInfo pipeline_rendering_info{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
    // TODO:
    // make this the same as what's used in begin_rendering
    pipeline_rendering_info.colorAttachmentCount = 1;
    pipeline_rendering_info.pColorAttachmentFormats = &swap_chain_defaults->surface_format().format;
    pipeline_rendering_info.depthAttachmentFormat = swap_chain_defaults->depth_format();

    auto _pipeline = std::make_shared<GraphicsPipeline>(pipeline_rendering_info, swap_chain_defaults->extent(), vert_path, frag_path);
    // NOTE:
    // push instead of emplace because of template errors
    // emplace calls shared_ptr, not make_shared
    _pipelines.push_back(_pipeline);

    // NOTE:
    // Technically passing _pipeline isn't needed here,
    // but it's being put here to ensure that the render node holds the lifetime of it
    add_node({_pipeline}, vkCmdBindPipeline, _pipeline->bind_point(), _pipeline->vk_pipeline());
    //    add_node(vkCmdBindDescriptorBuffersEXT, 1, _pipeline->descriptor_buffers_binding_info());

    uint32_t first_binding = 0;
    uint32_t binding_count = 0;
    // bind descriptors
    // push constants
    std::vector<VkBuffer> vertex_buffers = {MemoryManager::memory_manager->get_buffer(vertex_buffer_name)->vk_buffer()};
    std::vector<VkDeviceSize> offsets = {0};
    add_node(vkCmdBindVertexBuffers, first_binding, binding_count, vertex_buffers.data(), offsets.data());
    add_node(vkCmdBindIndexBuffer, MemoryManager::memory_manager->get_buffer(index_buffer_name)->vk_buffer(), 0, VK_INDEX_TYPE_UINT32);
}
