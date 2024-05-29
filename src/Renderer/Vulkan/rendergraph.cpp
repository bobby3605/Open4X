#include "rendergraph.hpp"
#include "common.hpp"
#include "device.hpp"
#include "memory_manager.hpp"
#include "pipeline.hpp"
#include "swapchain.hpp"
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <vulkan/vulkan_core.h>

RenderGraph::RenderGraph(VkCommandPool pool) : _pool{pool} {
    _descriptor_buffer_allocator = new LinearAllocator<GPUAllocator>(MemoryManager::memory_manager->create_gpu_allocator(
        "_descriptor_buffer_allocator", 1,
        VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT));
}

RenderGraph::~RenderGraph() {
    for (auto& graph : _primary_graphs) {
        Device::device->command_pools()->release_buffer(_pool, graph.first);
    }
    for (auto& graph : _secondary_graphs) {
        Device::device->command_pools()->release_buffer(_pool, graph.first);
    }
    // TODO:
    // Fix double free error so that this can be re-created
    // When the shader destructor is called, it frees the allocation
    // When this delete runs, it deletes the whole buffer,
    // which clears the virtual block
    // So there's a double free and then a segfault
    // MemoryManager::memory_manager->delete_buffer("descriptor_buffer");
}

void RenderGraph::compile() {
    // Bind descriptor buffer
    auto descriptor_buffers_binding_info = std::make_shared<VkDescriptorBufferBindingInfoEXT>();
    descriptor_buffers_binding_info->sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT;
    descriptor_buffers_binding_info->address = _descriptor_buffer_allocator->parent()->addr_info().address;
    descriptor_buffers_binding_info->usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;

    std::vector<RenderNode>& first_nodes = _primary_graphs.begin()->second;
    first_nodes.emplace(first_nodes.cbegin(), RenderDeps{descriptor_buffers_binding_info}, vkCmdBindDescriptorBuffersEXT_, 1,
                        descriptor_buffers_binding_info.get());

    // FIXME:
    // Only record non-per frame buffers
    for (auto& graph : _primary_graphs) {
        record_buffer(graph.first, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT, graph.second);
    }
    for (auto& graph : _secondary_graphs) {
        record_buffer(graph.first, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT, graph.second);
    }

    // Set descriptor buffer
    // FIXME:
    // Clean this up
    for (auto const& pipeline : _pipelines) {
        for (auto const& set_layout : pipeline->descriptor_layout().set_layouts()) {
            for (auto const& binding_layout : set_layout.second.bindings) {
                GPUAllocator* tmp;
                std::string buffer_name = binding_layout.second.buffer_name;
                if (MemoryManager::memory_manager->gpu_allocator_exists(buffer_name)) {
                    // TODO:
                    // Maybe
                    // support recreating buffer with new mem props if needed from the graph
                    // Other option is just to error out if you manually created a buffer with the wrong mem_props
                    tmp = MemoryManager::memory_manager->get_gpu_allocator(buffer_name);
                } else {
                    if (_buffer_size_registry.count(buffer_name) == 0) {
                        throw std::runtime_error("trying to create unregistered buffer: " + buffer_name);
                    }
                    tmp = MemoryManager::memory_manager->create_gpu_allocator(
                        buffer_name, 1,
                        type_to_usage(binding_layout.second.binding.descriptorType) | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                        binding_layout.second.mem_props | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
                }
                pipeline->descriptor_layout().set_descriptor_buffer(set_layout.first, binding_layout.first, tmp->descriptor_data());
            }
        }
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
    // FIXME:
    // Get inheritance info for dynamic rendering
    begin_info.pInheritanceInfo;
    check_result(vkBeginCommandBuffer(command_buffer, &begin_info), "failed to begin command buffer!");
    for (RenderNode const& render_node : render_nodes) {
        render_node.op(command_buffer);
    }
    check_result(vkEndCommandBuffer(command_buffer), "failed to end command buffer!");
}

void RenderGraph::bad_workaround(SwapChain* swap_chain) {
    VkCommandBuffer& cmd_buffer = _curr_primary_cmd_buffer;
    auto& nodes = _primary_graphs.at(cmd_buffer);

    uint32_t base_index_offset = 1;

    uint32_t current_transition_index = 0 + base_index_offset;
    uint32_t tmp_barrier_index = 0;
    auto tmp_barrier = reinterpret_cast<VkImageMemoryBarrier2*>(nodes[current_transition_index].deps[tmp_barrier_index].get());
    tmp_barrier->image = swap_chain->current_image();

    uint32_t depth_transition_index = current_transition_index + 1;
    tmp_barrier = reinterpret_cast<VkImageMemoryBarrier2*>(nodes[depth_transition_index].deps[tmp_barrier_index].get());
    tmp_barrier->image = swap_chain->depth_image();

    uint32_t begin_rendering_index = depth_transition_index + 1;
    uint32_t color_attachment_index = 0;
    auto color_attachment = reinterpret_cast<VkRenderingAttachmentInfo*>(nodes[begin_rendering_index].deps[color_attachment_index].get());
    color_attachment->imageView = swap_chain->color_image_view();

    uint32_t depth_attachment_index = 1;
    auto depth_attachment = reinterpret_cast<VkRenderingAttachmentInfo*>(nodes[begin_rendering_index].deps[depth_attachment_index].get());
    depth_attachment->imageView = swap_chain->depth_image_view();

    uint32_t bind_index_buffer_index = begin_rendering_index + 4;
    nodes[bind_index_buffer_index] =
        RenderNode(RenderDeps{}, vkCmdBindIndexBuffer,
                   MemoryManager::memory_manager->get_gpu_allocator("index_buffer")->base_alloc().buffer, 0, VK_INDEX_TYPE_UINT32);

    uint32_t present_transition_index = nodes.size() - 1;
    tmp_barrier = reinterpret_cast<VkImageMemoryBarrier2*>(nodes[present_transition_index].deps[tmp_barrier_index].get());
    tmp_barrier->image = swap_chain->current_image();

    record_buffer(cmd_buffer, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT, nodes);
}

VkResult RenderGraph::submit(SwapChain* swap_chain) {
    // Flatten primaries for submission
    // TODO
    // Move this into compile so it's only done once
    std::vector<VkCommandBuffer> primary_buffers;
    primary_buffers.reserve(_primary_graphs.size());
    for (auto& graph : _primary_graphs) {
        primary_buffers.push_back(graph.first);
    }
    for (auto& cmd_buffer : _per_frame_primary) {
        record_buffer(cmd_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, _primary_graphs.at(cmd_buffer));
    }
    for (auto& cmd_buffer : _per_frame_secondary) {
        record_buffer(cmd_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, _secondary_graphs.at(cmd_buffer));
    }
    // FIXME:
    // Sets the per-frame values
    bad_workaround(swap_chain);
    // Return true if swapchain recreated
    return swap_chain->submit_command_buffers(primary_buffers);
}

// TODO
// Figure out the primary vs secondary buffers and re-recording at graph compile time
void RenderGraph::define_primary(bool per_frame) {
    // TODO
    // Look into replacing _curr with _graphs.end()
    // unordered map should work fine
    _curr_primary_cmd_buffer = Device::device->command_pools()->get_primary(_pool);
    Device::device->set_debug_name(VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)_curr_primary_cmd_buffer,
                                   "primary_cmd_buffer[" + std::to_string(_primary_graphs.size()) + "]");
    if (per_frame) {
        _per_frame_primary.push_back(_curr_primary_cmd_buffer);
    }
    _secondary = false;
}

void RenderGraph::define_secondary(bool per_frame) {
    _curr_secondary_cmd_buffer = Device::device->command_pools()->get_secondary(_pool);
    Device::device->set_debug_name(VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)_curr_secondary_cmd_buffer,
                                   "secondary_cmd_buffer[" + std::to_string(_secondary_graphs.size()) + "]");
    auto secondary_lifetime = std::make_shared<VkCommandBuffer>(_curr_secondary_cmd_buffer);
    // TODO
    // optimize this in compile
    // if multiple secondary command buffers come right after one another,
    // they can be combined into one execute commands if there's no memory hazards

    // Add secondary to current primary command buffer
    add_node({secondary_lifetime}, vkCmdExecuteCommands, 1, secondary_lifetime.get());
    _secondary = true;
    if (per_frame) {
        _per_frame_secondary.push_back(_curr_secondary_cmd_buffer);
    }
}

// FIXME:
// Handle resized or re-created swapchain
// Will need to re-record buffer,
// maybe put vkCmdBeginRendering into a secondary buffer? so that the whole thing doesn't need to be re-recorded
void RenderGraph::begin_rendering(SwapChain* const swap_chain) {
    transition_image(swap_chain->current_image(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                     VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

    transition_image(swap_chain->depth_image(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                     VkImageSubresourceRange{VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1});

    auto color_attachment = std::make_shared<VkRenderingAttachmentInfo>();
    color_attachment->sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    // NOTE:
    // This probably needs to be a reference to the image that the swapchain is currently using,
    // more than 1 frame in flight will break this because it's pre-recorded
    // Or maybe it's fine, because we only process one frame at a time??
    color_attachment->imageView = swap_chain->current_image_view();
    color_attachment->imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attachment->loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment->clearValue.color = {0.0f, 0.0f, 0.0f, 1.0f};

    auto depth_attachment = std::make_shared<VkRenderingAttachmentInfo>();
    depth_attachment->sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depth_attachment->imageView = swap_chain->depth_image_view();
    depth_attachment->imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depth_attachment->loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment->storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment->clearValue.depthStencil = {1.0f, 0};

    auto pass_info = std::make_shared<VkRenderingInfo>();
    pass_info->sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    pass_info->renderArea.extent = swap_chain->extent();
    pass_info->layerCount = 1;
    pass_info->colorAttachmentCount = 1;
    pass_info->pColorAttachments = color_attachment.get();
    pass_info->pDepthAttachment = depth_attachment.get();

    add_node({color_attachment, depth_attachment, pass_info}, vkCmdBeginRendering, pass_info.get());
}
void RenderGraph::end_rendering(SwapChain* const swap_chain) {
    add_node(vkCmdEndRendering);
    transition_image(swap_chain->current_image(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                     VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});
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
    auto dependency_info = std::make_shared<VkDependencyInfo>();

    dependency_info->sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependency_info->dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    dependency_info->imageMemoryBarrierCount = 1;
    dependency_info->pImageMemoryBarriers = tmp_barrier.get();

    add_node({tmp_barrier, dependency_info}, vkCmdPipelineBarrier2, dependency_info.get());
}

void RenderGraph::graphics_pass(std::string const& vert_path, std::string const& frag_path,
                                LinearAllocator<GPUAllocator>* vertex_buffer_allocator,
                                LinearAllocator<GPUAllocator>* index_buffer_allocator, SwapChain* swap_chain_defaults) {

    VkPipelineRenderingCreateInfo pipeline_rendering_info{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
    // TODO:
    // make this the same as what's used in begin_rendering
    pipeline_rendering_info.colorAttachmentCount = 1;
    pipeline_rendering_info.pColorAttachmentFormats = &swap_chain_defaults->surface_format().format;
    pipeline_rendering_info.depthAttachmentFormat = swap_chain_defaults->depth_format();

    auto pipeline = std::make_shared<GraphicsPipeline>(pipeline_rendering_info, swap_chain_defaults->extent(), vert_path, frag_path,
                                                       _descriptor_buffer_allocator);
    // NOTE:
    // push instead of emplace because of template errors
    // emplace calls shared_ptr, not make_shared
    _pipelines.push_back(pipeline);

    add_node(vkCmdBindPipeline, pipeline->bind_point(), pipeline->vk_pipeline());

    // Set descriptor offsets
    auto descriptor_buffer_offsets = std::make_shared<VkSetDescriptorBufferOffsetsInfoEXT>();
    descriptor_buffer_offsets->sType = VK_STRUCTURE_TYPE_SET_DESCRIPTOR_BUFFER_OFFSETS_INFO_EXT;
    descriptor_buffer_offsets->stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    descriptor_buffer_offsets->layout = pipeline->vk_pipeline_layout();
    // TODO
    // Only update needed sets
    // Example:
    // global set at 0
    // vertex and fragment at 1 and 2
    // So only 1 and 2 need to be updated
    descriptor_buffer_offsets->firstSet = 0;
    auto set_offsets = std::make_shared<std::vector<VkDeviceSize>>(pipeline->descriptor_layout().set_offsets());
    descriptor_buffer_offsets->setCount = set_offsets->size();
    auto buffer_indices = std::make_shared<std::vector<uint32_t>>(descriptor_buffer_offsets->setCount);
    for (auto const& buffer_index : *buffer_indices) {
        buffer_indices->push_back(0);
    }
    descriptor_buffer_offsets->pOffsets = set_offsets->data();
    descriptor_buffer_offsets->pBufferIndices = buffer_indices->data();

    add_node({descriptor_buffer_offsets, set_offsets, buffer_indices}, vkCmdSetDescriptorBufferOffsets2EXT_,
             descriptor_buffer_offsets.get());

    // push constants

    // set vertex and index buffers
    auto offsets = std::make_shared<std::vector<VkDeviceSize>>(1, 0);
    // FIXME:
    // support gpu_allocator reallocs
    add_node({offsets}, vkCmdBindVertexBuffers, 0, 1, &vertex_buffer_allocator->base_alloc().buffer, offsets->data());
    add_node(vkCmdBindIndexBuffer, index_buffer_allocator->base_alloc().buffer, 0, VK_INDEX_TYPE_UINT32);
}

void RenderGraph::buffer(std::string name, VkDeviceSize size) { _buffer_size_registry.insert_or_assign(name, size); }
