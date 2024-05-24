#include "pipeline.hpp"
#include "common.hpp"
#include "device.hpp"
#include "memory_manager.hpp"
#include "model.hpp"
#include <cstddef>
#include <cstdlib>
#include <unordered_map>
#include <vulkan/vulkan_core.h>

Pipeline::~Pipeline() {
    vkDestroyPipelineLayout(Device::device->vk_device(), _pipeline_layout, nullptr);
    vkDestroyPipeline(Device::device->vk_device(), _pipeline, nullptr);
}

GraphicsPipeline::~GraphicsPipeline() {
    // NOTE:
    // ~Pipeline() should get called after this
}

GraphicsPipeline::GraphicsPipeline(VkPipelineRenderingCreateInfo& pipeline_rendering_info, VkExtent2D extent, std::string const& vert_path,
                                   std::string const& frag_path) {
    _bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;
    _pipeline_name = get_filename_no_ext(vert_path);

    VkPipelineInputAssemblyStateCreateInfo input_assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = extent.height;
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
    pipeline_info.flags = VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;

    // create shaders
    _shaders.emplace("vert", vert_path);
    _shaders.emplace("frag", frag_path);

    /*
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstants);
    */

    // pipelineLayoutInfo.pushConstantRangeCount = 1;
    //  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    std::vector<VkPipelineShaderStageCreateInfo> shader_stages;
    std::vector<VkDescriptorSetLayout> descriptor_buffer_layouts;
    // Copy stages and descriptor layouts into contiguous memory
    // Get descriptor buffer size
    for (auto const& shader : shaders()) {
        shader_stages.push_back(shader.second.stage_info());
        // TODO
        // Make this better, so there's less copying
        // c++23 has append_range, but I can't get it to compile
        std::vector<VkDescriptorSetLayout> tmp = shader.second.descriptor_layout().vk_set_layouts();
        descriptor_buffer_layouts.insert(descriptor_buffer_layouts.end(), tmp.begin(), tmp.end());
    }

    VkPipelineLayoutCreateInfo pipeline_layout_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipeline_layout_info.setLayoutCount = descriptor_buffer_layouts.size();
    pipeline_layout_info.pSetLayouts = descriptor_buffer_layouts.data();

    check_result(vkCreatePipelineLayout(Device::device->vk_device(), &pipeline_layout_info, nullptr, &_pipeline_layout),
                 "failed to create graphics pipeline layout");
    Device::device->set_debug_name(VK_OBJECT_TYPE_PIPELINE_LAYOUT, (uint64_t)_pipeline_layout, _pipeline_name + "_layout");

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
