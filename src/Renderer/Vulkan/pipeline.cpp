#include "pipeline.hpp"
#include "../Allocator/base_allocator.hpp"
#include "common.hpp"
#include "device.hpp"
#include "draw.hpp"
#include "memory_manager.hpp"
#include "model.hpp"
#include "shader.hpp"
#include <list>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vulkan/vulkan_core.h>

Pipeline::Pipeline() {}

Pipeline::Pipeline(LinearAllocator<GPUAllocation>* descriptor_buffer_allocator) : _descriptor_layout(descriptor_buffer_allocator) {}

Pipeline::~Pipeline() {
    for (Shader*& shader : _shaders) {
        delete shader;
    }
    vkDestroyPipelineLayout(Device::device->vk_device(), _pipeline_layout, nullptr);
    vkDestroyPipeline(Device::device->vk_device(), _pipeline, nullptr);
}

void Pipeline::load_shaders(std::vector<std::filesystem::path> const& shader_paths, std::vector<void*> const& specialization_data) {
    for (size_t i = 0; i < shader_paths.size(); ++i) {
        // create shaders
        _shaders.push_back(new Shader(shader_paths[i], &_descriptor_layout, specialization_data[i]));
    }

    // handle push constants
    // Copy stages and descriptor layouts into contiguous memory
    std::vector<VkPushConstantRange> push_constant_ranges;
    for (Shader* const& shader : shaders()) {
        if (shader->has_push_constants()) {
            push_constant_ranges.push_back(shader->push_constant_range());
        }
        _shader_stages.push_back(shader->stage_info());
    }

    // Get descriptor buffer size
    _descriptor_layout.create_layouts();
    std::vector<VkDescriptorSetLayout> descriptor_buffer_layouts = _descriptor_layout.vk_set_layouts();

    VkPipelineLayoutCreateInfo pipeline_layout_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipeline_layout_info.setLayoutCount = descriptor_buffer_layouts.size();
    pipeline_layout_info.pSetLayouts = descriptor_buffer_layouts.data();
    pipeline_layout_info.pushConstantRangeCount = push_constant_ranges.size();
    pipeline_layout_info.pPushConstantRanges = push_constant_ranges.data();

    check_result(vkCreatePipelineLayout(Device::device->vk_device(), &pipeline_layout_info, nullptr, &_pipeline_layout),
                 "failed to create graphics pipeline layout");
    Device::device->set_debug_name(VK_OBJECT_TYPE_PIPELINE_LAYOUT, (uint64_t)_pipeline_layout, _pipeline_name + "_layout");
}

GraphicsPipeline::~GraphicsPipeline() {
    // NOTE:
    // ~Pipeline() should get called after this
}

GraphicsPipeline::GraphicsPipeline(VkPipelineRenderingCreateInfo& pipeline_rendering_info, VkExtent2D extent,
                                   std::filesystem::path const& vert_path, std::filesystem::path const& frag_path, void* vert_spec_data,
                                   void* frag_spec_data) {
    create(pipeline_rendering_info, extent, vert_path, frag_path, vert_spec_data, frag_spec_data);
}

GraphicsPipeline::GraphicsPipeline(VkPipelineRenderingCreateInfo& pipeline_rendering_info, VkExtent2D extent,
                                   std::filesystem::path const& vert_path, std::filesystem::path const& frag_path, void* vert_spec_data,
                                   void* frag_spec_data, LinearAllocator<GPUAllocation>* descriptor_buffer_allocator)
    : Pipeline(descriptor_buffer_allocator) {
    create(pipeline_rendering_info, extent, vert_path, frag_path, vert_spec_data, frag_spec_data);
}

void GraphicsPipeline::create(VkPipelineRenderingCreateInfo& pipeline_rendering_info, VkExtent2D extent,
                              std::filesystem::path const& vert_path, std::filesystem::path const& frag_path, void* vert_spec_data,
                              void* frag_spec_data) {
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

    load_shaders({vert_path, frag_path}, {vert_spec_data, frag_spec_data});

    // FIXME:
    // Get this from the vertex shader
    auto binding_description = NewVertex::binding_description();
    auto attribute_descriptions = NewVertex::attribute_descriptions();

    VkPipelineVertexInputStateCreateInfo vertex_input_info{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.pVertexBindingDescriptions = &binding_description;
    vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
    vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data();

    pipeline_info.stageCount = _shader_stages.size();
    pipeline_info.pStages = _shader_stages.data();
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.layout = _pipeline_layout;

    check_result(vkCreateGraphicsPipelines(Device::device->device->vk_device(), VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &_pipeline),
                 "failed to create graphics pipeline");
    Device::device->set_debug_name(VK_OBJECT_TYPE_PIPELINE, (uint64_t)_pipeline, _pipeline_name);
}

ComputePipeline::ComputePipeline(std::filesystem::path const& compute_path, void* compute_spec_data) {
    create(compute_path, compute_spec_data);
}

ComputePipeline::ComputePipeline(std::filesystem::path const& compute_path, void* compute_spec_data,
                                 LinearAllocator<GPUAllocation>* descriptor_buffer_allocator)
    : Pipeline(descriptor_buffer_allocator) {
    create(compute_path, compute_spec_data);
}

void ComputePipeline::create(std::filesystem::path const& compute_path, void* compute_spec_data) {
    _bind_point = VK_PIPELINE_BIND_POINT_COMPUTE;
    _pipeline_name = get_filename_no_ext(compute_path);

    load_shaders({compute_path}, {compute_spec_data});

    // NOTE:
    // force subgroup size to maximum
    // only needed on intel
    VkPipelineShaderStageRequiredSubgroupSizeCreateInfo subgroup_size_info{
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO};
    subgroup_size_info.requiredSubgroupSize = Device::device->max_subgroup_size();

    VkComputePipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_info.layout = _pipeline_layout;

    pipeline_info.pNext = VK_NULL_HANDLE;
    pipeline_info.stage = _shader_stages[0];
    pipeline_info.stage.pNext = &subgroup_size_info;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.basePipelineIndex = 0;

    if (Device::device->use_descriptor_buffers()) {
        pipeline_info.flags = VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
    }

    check_result(vkCreateComputePipelines(Device::device->device->vk_device(), VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &_pipeline),
                 "failed to create compute pipeline");
    Device::device->set_debug_name(VK_OBJECT_TYPE_PIPELINE, (uint64_t)_pipeline, _pipeline_name);
}
