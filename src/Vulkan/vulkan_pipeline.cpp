#include "vulkan_pipeline.hpp"
#include "common.hpp"
#include "vulkan_descriptors.hpp"
#include "vulkan_model.hpp"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

GraphicsPipeline::GraphicsPipeline(std::shared_ptr<VulkanDevice> device, VulkanSwapChain* swapChain,
                                   VkPipelineShaderStageCreateInfo vertInfo, VkPipelineShaderStageCreateInfo fragInfo,
                                   std::vector<VkDescriptorSetLayout>& descriptorLayouts, std::vector<VkPushConstantRange>& pushConstants)
    : VulkanPipeline(device) {

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = swapChain->getExtent().height;
    viewport.width = swapChain->getExtent().width;
    viewport.height = swapChain->getExtent().height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChain->getExtent();

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
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

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = device->getMsaaSamples();
    multisampling.sampleShadingEnable = device->getSampleShading();
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {};
    depthStencil.back = {};

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    // set in VulkanPipeline::createGraphicsPipeline()
    pipelineInfo.pStages = nullptr;
    pipelineInfo.pVertexInputState = nullptr;
    //
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    // VK_DYNAMIC_STATE_VIEWPORT specifies that the pViewports state in
    // VkPipelineViewportStateCreateInfo will be ignored and must be set
    // dynamically with vkCmdSetViewport before any drawing commands. The number
    // of viewports used by a pipeline is still specified by the viewportCount
    // member of VkPipelineViewportStateCreateInfo.
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;
    pipelineInfo.pDepthStencilState = &depthStencil;
    VkPipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = 1;
    VkFormat colorFormat = swapChain->getSwapChainImageFormat();
    renderingInfo.pColorAttachmentFormats = &colorFormat;
    renderingInfo.depthAttachmentFormat = swapChain->findDepthFormat();
    pipelineInfo.pNext = &renderingInfo;
    /*
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstants);
    */

    /*
    std::vector<VkDescriptorSetLayout> descriptorLayouts;
    descriptorLayouts.push_back(descriptorManager->descriptors["global"]->getLayout());
    descriptorLayouts.push_back(descriptorManager->descriptors["material"]->getLayout());
    descriptorLayouts.push_back(descriptorManager->descriptors["object"]->getLayout());
    */

    /*
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = descriptorLayouts.size();
    pipelineLayoutInfo.pSetLayouts = descriptorLayouts.data();
    */
    // pipelineLayoutInfo.pushConstantRangeCount = 1;
    //  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    /*
    checkResult(vkCreatePipelineLayout(_device->device(), &pipelineLayoutInfo, nullptr, &_pipelineLayout),
                "failed to create graphics pipeline layout");

    std::string baseShaderPath = "assets/shaders/";
    std::string baseShaderName = "triangle";

    std::string vertShaderPath = baseShaderPath + baseShaderName + ".vert";
    std::string fragShaderPath = baseShaderPath + baseShaderName + ".frag";

    std::vector<uint32_t> vertShaderCode = compileShader(vertShaderPath);
    std::vector<uint32_t> fragShaderCode = compileShader(fragShaderPath);

    VkShaderModule vertModule = createShaderModule(vertShaderCode);
    VkShaderModule fragModule = createShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragModule;
    fragShaderStageInfo.pName = "main";
    */

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertInfo, fragInfo};

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    createPipelineLayout(descriptorLayouts, pushConstants);
    pipelineInfo.layout = _pipelineLayout;

    checkResult(vkCreateGraphicsPipelines(device->device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline),
                "failed to create graphics pipeline");

    /*
    vkDestroyShaderModule(device->device(), vertModule, nullptr);
    vkDestroyShaderModule(device->device(), fragModule, nullptr);
    */
}

void VulkanPipeline::createPipelineLayout(std::vector<VkDescriptorSetLayout>& descriptorLayouts,
                                          std::vector<VkPushConstantRange>& pushConstants) {
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = descriptorLayouts.size();
    pipelineLayoutInfo.pSetLayouts = descriptorLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = pushConstants.size();
    pipelineLayoutInfo.pPushConstantRanges = pushConstants.data();

    checkResult(vkCreatePipelineLayout(_device->device(), &pipelineLayoutInfo, nullptr, &_pipelineLayout),
                "failed to create compute pipeline layout");
}

ComputePipeline::ComputePipeline(std::shared_ptr<VulkanDevice> device, std::vector<VkDescriptorSetLayout>& descriptorLayouts,
                                 std::vector<VkPushConstantRange>& pushConstants, VkPipelineShaderStageCreateInfo stageInfo)
    : VulkanPipeline(device) {

    createPipelineLayout(descriptorLayouts, pushConstants);

    /*
     */
    // NOTE:
    // force subgroup size to maximum
    // only needed on intel
    VkPipelineShaderStageRequiredSubgroupSizeCreateInfo subgroupSizeInfo{};
    subgroupSizeInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO;
    subgroupSizeInfo.requiredSubgroupSize = _device->maxSubgroupSize();

    stageInfo.pNext = &subgroupSizeInfo;

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = _pipelineLayout;

    pipelineInfo.pNext = VK_NULL_HANDLE;
    pipelineInfo.stage = stageInfo;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = 0;

    checkResult(vkCreateComputePipelines(_device->device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline),
                "failed to create compute pipeline");
}

VulkanPipeline::~VulkanPipeline() {
    if (_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(_device->device(), _pipelineLayout, nullptr);
    }
    vkDestroyPipeline(_device->device(), _pipeline, nullptr);
}
