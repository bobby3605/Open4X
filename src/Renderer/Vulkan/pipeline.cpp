#include "pipeline.hpp"
#include "common.hpp"
#include "device.hpp"
#include "memory_manager.hpp"
#include "model.hpp"
#include <iostream>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vulkan/vulkan_core.h>

Pipeline::Pipeline() {}

Pipeline::Pipeline(LinearAllocator<GPUAllocator>* descriptor_buffer_allocator) : _descriptor_layout(descriptor_buffer_allocator) {}

Pipeline::~Pipeline() {
    vkDestroyPipelineLayout(Device::device->vk_device(), _pipeline_layout, nullptr);
    vkDestroyPipeline(Device::device->vk_device(), _pipeline, nullptr);
}

void Pipeline::update_descriptors() {
    // FIXME:
    // Support only re-binding needed sets
    // For example, a global set at 0 only needs to be set once per GPUAllocator->realloc
    for (auto const& set_layout : _descriptor_layout.set_layouts()) {
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
                tmp = MemoryManager::memory_manager->create_gpu_allocator(
                    buffer_name, 1, type_to_usage(binding_layout.second.binding.descriptorType) | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                    binding_layout.second.mem_props | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
            }
            if (Device::device->use_descriptor_buffers()) {
                _descriptor_layout.set_descriptor_buffer(set_layout.first, binding_layout.first, tmp->descriptor_data());
            } else {
                VkWriteDescriptorSet descriptor_write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
                descriptor_write.dstSet = set_layout.second.set;
                descriptor_write.dstBinding = binding_layout.second.binding.binding;
                descriptor_write.dstArrayElement = 0;
                descriptor_write.descriptorType = binding_layout.second.binding.descriptorType;
                descriptor_write.descriptorCount = 1;
                VkDescriptorBufferInfo buffer_info{};
                buffer_info.buffer = tmp->base_alloc().buffer;
                buffer_info.offset = 0;
                buffer_info.range = VK_WHOLE_SIZE;
                switch (descriptor_write.descriptorType) {
                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                    descriptor_write.pBufferInfo = &buffer_info;
                    break;
                case VK_DESCRIPTOR_TYPE_SAMPLER:
                case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                    // FIXME:
                    descriptor_write.pImageInfo;
                    break;
                default:
                    throw std::runtime_error("unknown descriptor type");
                }
                // FIXME:
                // Allocate all sets in one call,
                // the problem is that descriptor_write.pBufferInfo is a pointer,
                // so a new VkDescriptorBufferInfo needs to be created for each descriptor_write, then destroyed
                vkUpdateDescriptorSets(Device::device->vk_device(), 1, &descriptor_write, 0, nullptr);
            }
        }
    }
}

GraphicsPipeline::~GraphicsPipeline() {
    // NOTE:
    // ~Pipeline() should get called after this
}
GraphicsPipeline::GraphicsPipeline(VkPipelineRenderingCreateInfo& pipeline_rendering_info, VkExtent2D extent, std::string const& vert_path,
                                   std::string const& frag_path) {
    create(pipeline_rendering_info, extent, vert_path, frag_path);
}

GraphicsPipeline::GraphicsPipeline(VkPipelineRenderingCreateInfo& pipeline_rendering_info, VkExtent2D extent, std::string const& vert_path,
                                   std::string const& frag_path, LinearAllocator<GPUAllocator>* descriptor_buffer_allocator)
    : Pipeline(descriptor_buffer_allocator) {
    create(pipeline_rendering_info, extent, vert_path, frag_path);
}

void GraphicsPipeline::create(VkPipelineRenderingCreateInfo& pipeline_rendering_info, VkExtent2D extent, std::string const& vert_path,
                              std::string const& frag_path) {
    _bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;
    _pipeline_name = get_filename_no_ext(vert_path);

    VkPipelineInputAssemblyStateCreateInfo input_assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = extent.width;
    viewport.height = extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = extent;

    VkPipelineViewportStateCreateInfo viewport_state{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewport_state.viewportCount = 1;
    viewport_state.pViewports = &viewport;
    viewport_state.scissorCount = 1;
    viewport_state.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    // NOTE:
    // gltf spec guarantees counter clockwise winding order
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisampling{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = Device::device->msaa_samples();
    multisampling.sampleShadingEnable = Device::device->sample_shading;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo color_blending{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.logicOp = VK_LOGIC_OP_COPY;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &color_blend_attachment;
    color_blending.blendConstants[0] = 0.0f;
    color_blending.blendConstants[1] = 0.0f;
    color_blending.blendConstants[2] = 0.0f;
    color_blending.blendConstants[3] = 0.0f;

    std::vector<VkDynamicState> dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamic_state{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamic_state.pDynamicStates = dynamic_states.data();

    VkPipelineDepthStencilStateCreateInfo depth_stencil{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    depth_stencil.depthTestEnable = VK_TRUE;
    depth_stencil.depthWriteEnable = VK_TRUE;
    depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil.depthBoundsTestEnable = VK_FALSE;
    depth_stencil.minDepthBounds = 0.0f;
    depth_stencil.maxDepthBounds = 1.0f;
    depth_stencil.stencilTestEnable = VK_FALSE;
    depth_stencil.front = {};
    depth_stencil.back = {};

    VkGraphicsPipelineCreateInfo pipeline_info{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipeline_info.pStages = nullptr;
    pipeline_info.pVertexInputState = nullptr;
    //
    pipeline_info.pInputAssemblyState = &input_assembly;
    // VK_DYNAMIC_STATE_VIEWPORT specifies that the pViewports state in
    // VkPipelineViewport_stateCreateInfo will be ignored and must be set
    // dynamically with vkCmdSetViewport before any drawing commands. The number
    // of viewports used by a pipeline is still specified by the viewportCount
    // member of VkPipelineViewport_stateCreateInfo.
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pDepthStencilState = nullptr;
    pipeline_info.pColorBlendState = &color_blending;
    pipeline_info.pDynamicState = &dynamic_state;
    pipeline_info.subpass = 0;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.basePipelineIndex = -1;
    pipeline_info.pDepthStencilState = &depth_stencil;
    pipeline_info.pNext = &pipeline_rendering_info;
    if (Device::device->use_descriptor_buffers()) {
        pipeline_info.flags = VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
    }

    // create shaders
    _shaders.emplace(std::piecewise_construct, std::forward_as_tuple("vert"), std::forward_as_tuple(vert_path, &_descriptor_layout));
    _shaders.emplace(std::piecewise_construct, std::forward_as_tuple("frag"), std::forward_as_tuple(frag_path, &_descriptor_layout));

    /*
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstants);
    */

    // pipelineLayoutInfo.pushConstantRangeCount = 1;
    //  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    std::vector<VkPipelineShaderStageCreateInfo> shader_stages;
    // Copy stages and descriptor layouts into contiguous memory
    // Get descriptor buffer size
    for (auto const& shader : shaders()) {
        shader_stages.push_back(shader.second.stage_info());
    }

    _descriptor_layout.create_layouts();
    std::vector<VkDescriptorSetLayout> descriptor_buffer_layouts = _descriptor_layout.vk_set_layouts();

    VkPipelineLayoutCreateInfo pipeline_layout_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipeline_layout_info.setLayoutCount = descriptor_buffer_layouts.size();
    pipeline_layout_info.pSetLayouts = descriptor_buffer_layouts.data();

    check_result(vkCreatePipelineLayout(Device::device->vk_device(), &pipeline_layout_info, nullptr, &_pipeline_layout),
                 "failed to create graphics pipeline layout");
    Device::device->set_debug_name(VK_OBJECT_TYPE_PIPELINE_LAYOUT, (uint64_t)_pipeline_layout, _pipeline_name + "_layout");

    // FIXME:
    // Get this from the vertex shader
    auto binding_description = NewVertex::binding_description();
    auto attribute_descriptions = NewVertex::attribute_descriptions();

    VkPipelineVertexInputStateCreateInfo vertex_input_info{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.pVertexBindingDescriptions = &binding_description;
    vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
    vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data();

    pipeline_info.stageCount = shader_stages.size();
    pipeline_info.pStages = shader_stages.data();
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.layout = _pipeline_layout;

    check_result(vkCreateGraphicsPipelines(Device::device->device->vk_device(), VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &_pipeline),
                 "failed to create graphics pipeline");
    Device::device->set_debug_name(VK_OBJECT_TYPE_PIPELINE, (uint64_t)_pipeline, _pipeline_name);
}
