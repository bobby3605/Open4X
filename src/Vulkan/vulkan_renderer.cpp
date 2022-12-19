#include "vulkan_renderer.hpp"
#include "vulkan_descriptors.hpp"
#include "vulkan_device.hpp"
#include <cstdint>
#include <vector>
#include <vulkan/vulkan_core.h>
#define STB_IMAGE_IMPLEMENTATION
#include "../../external/stb_image.h"
#include "common.hpp"
#include "vulkan_swapchain.hpp"
#include "vulkan_window.hpp"
#include <array>
#include <cstring>
#include <iostream>
#include <stdexcept>

VulkanRenderer::VulkanRenderer(VulkanWindow* window, VulkanDevice* deviceRef, VulkanDescriptors* descriptorManager)
    : vulkanWindow{window}, device{deviceRef}, descriptorManager{descriptorManager} {
    init();
}

VulkanRenderer::~VulkanRenderer() {
    delete computePipeline;
    vkDestroyPipelineLayout(device->device(), computePipelineLayout, nullptr);
    delete graphicsPipeline;
    vkDestroyPipelineLayout(device->device(), pipelineLayout, nullptr);
    delete swapChain;
}

void VulkanRenderer::init() {
    swapChain = new VulkanSwapChain(device, vulkanWindow->getExtent());
    createCommandBuffers();
    createComputePipeline();
    createPipeline();
}

void VulkanRenderer::bindPipeline() {
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.width = swapChain->getExtent().width;
#ifdef FLIP_VIEWPORT
    viewport.y = swapChain->getExtent().height;
    viewport.height = -(float)swapChain->getExtent().height;
#else
    viewport.y = 0.0f;
    viewport.height = swapChain->getExtent().height;
#endif
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vkCmdSetViewport(getCurrentCommandBuffer(), 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChain->getExtent();

    vkCmdSetScissor(getCurrentCommandBuffer(), 0, 1, &scissor);

    vkCmdBindPipeline(getCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline->getPipeline());
}

void VulkanRenderer::bindComputePipeline() {
    vkCmdBindPipeline(getCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline->getPipeline());
}

void VulkanRenderer::bindDescriptorSet(VkPipelineBindPoint bindPoint, VkPipelineLayout layout, uint32_t setNum, VkDescriptorSet set) {
    vkCmdBindDescriptorSets(getCurrentCommandBuffer(), bindPoint, layout, setNum, 1, &set, 0, nullptr);
}

void VulkanRenderer::recreateSwapChain() {
    // pause on minimization
    /*
    int width = 0, height = 0;
    while (width == 0 || height == 0) {
      glfwGetFramebufferSize(vulkanWindow->getGLFWwindow(), &width, &height);
      glfwWaitEvents();
    }
    */

    vkDeviceWaitIdle(device->device());

    swapChain = new VulkanSwapChain(device, vulkanWindow->getExtent(), swapChain);
}

void VulkanRenderer::createComputePipeline() {
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(ComputePushConstants);

    std::vector<VkDescriptorSetLayout> descriptorLayouts;
    descriptorLayouts.push_back(descriptorManager->descriptors["compute"]->getLayout());

    VkPipelineLayoutCreateInfo computePipelineLayoutInfo{};
    computePipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    computePipelineLayoutInfo.setLayoutCount = descriptorLayouts.size();
    computePipelineLayoutInfo.pSetLayouts = descriptorLayouts.data();
    computePipelineLayoutInfo.pushConstantRangeCount = 1;
    computePipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    checkResult(vkCreatePipelineLayout(device->device(), &computePipelineLayoutInfo, nullptr, &computePipelineLayout),
                "failed to create compute pipeline layout");

    VkComputePipelineCreateInfo computePipelineInfo{};
    computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineInfo.layout = computePipelineLayout;

    computePipeline = new VulkanPipeline(device, computePipelineInfo);
}

void VulkanRenderer::createPipeline() {
    /*
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstants);
    */

    // FIXME:
    // fix the descriptor here for the new allocation method
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = descriptorManager->graphicsDescriptorLayouts.size();
    pipelineLayoutInfo.pSetLayouts = descriptorManager->graphicsDescriptorLayouts.data();
    // pipelineLayoutInfo.pushConstantRangeCount = 1;
    //  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    checkResult(vkCreatePipelineLayout(device->device(), &pipelineLayoutInfo, nullptr, &pipelineLayout),
                "failed to create pipeline layout");

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
    pipelineInfo.layout = pipelineLayout;
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

    graphicsPipeline = new VulkanPipeline(device, pipelineInfo);
}

void VulkanRenderer::createCommandBuffers() {
    commandBuffers.resize(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = device->getCommandPool();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

    checkResult(vkAllocateCommandBuffers(device->device(), &allocInfo, commandBuffers.data()), "failed to create command buffers");
}
// https://github.com/zeux/niagara/blob/master/src/shaders.h#L38
inline uint32_t getGroupCount(uint32_t threadCount, uint32_t localSize) { return (threadCount + localSize - 1) / localSize; }

void VulkanRenderer::runComputePipeline(VkDescriptorSet computeSet, VkBuffer indirectCountBuffer,
                                        ComputePushConstants& computePushConstants) {

    vkCmdFillBuffer(getCurrentCommandBuffer(), indirectCountBuffer, 0, sizeof(uint32_t), 0);

    VkMemoryBarrier2 indirectCountBarrier{};
    indirectCountBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
    indirectCountBarrier.pNext = VK_NULL_HANDLE;
    indirectCountBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    indirectCountBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_COPY_BIT;
    indirectCountBarrier.dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
    indirectCountBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;

    VkDependencyInfo indirectCountDependencyInfo{};
    indirectCountDependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    indirectCountDependencyInfo.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    indirectCountDependencyInfo.memoryBarrierCount = 1;
    indirectCountDependencyInfo.pMemoryBarriers = &indirectCountBarrier;

    vkCmdPipelineBarrier2(getCurrentCommandBuffer(), &indirectCountDependencyInfo);

    bindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, computeSet);

    vkCmdPushConstants(getCurrentCommandBuffer(), computePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants),
                       &computePushConstants);

    vkCmdDispatch(getCurrentCommandBuffer(), getGroupCount(computePushConstants.drawIndirectCount, 64), 1, 1);

    VkMemoryBarrier2 computeBarrier{};
    computeBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
    computeBarrier.pNext = VK_NULL_HANDLE;
    computeBarrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
    computeBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    computeBarrier.dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
    // TODO:
    // there may be a better dstStageMask to use here
    computeBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;

    VkDependencyInfo computeDependencyInfo{};
    computeDependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    computeDependencyInfo.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    computeDependencyInfo.memoryBarrierCount = 1;
    computeDependencyInfo.pMemoryBarriers = &computeBarrier;

    vkCmdPipelineBarrier2(getCurrentCommandBuffer(), &computeDependencyInfo);
}

void VulkanRenderer::beginRendering() {
    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = swapChain->getSwapChainImageView();
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue.color = {0.0f, 0.0f, 0.0f, 1.0f};

    VkRenderingAttachmentInfo depthAttachment{};
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.imageView = swapChain->getDepthImageView();
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.clearValue.depthStencil = {1.0f, 0};

    VkRenderingInfo passInfo{};
    passInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    passInfo.renderArea.extent = swapChain->getExtent();
    passInfo.layerCount = 1;
    passInfo.colorAttachmentCount = 1;
    passInfo.pColorAttachments = &colorAttachment;
    passInfo.pDepthAttachment = &depthAttachment;

    device->transitionImageLayout(swapChain->getSwapChainImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                  VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}, getCurrentCommandBuffer());

    device->transitionImageLayout(swapChain->getDepthImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                  VkImageSubresourceRange{VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1}, getCurrentCommandBuffer());

    vkCmdBeginRendering(getCurrentCommandBuffer(), &passInfo);
}

void VulkanRenderer::endRendering() {
    vkCmdEndRendering(getCurrentCommandBuffer());

    device->transitionImageLayout(swapChain->getSwapChainImage(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                  VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}, getCurrentCommandBuffer());
}

void VulkanRenderer::startFrame() {
    VkResult result = swapChain->acquireNextImage();

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image");
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;

    vkResetCommandBuffer(getCurrentCommandBuffer(), 0);

    checkResult(vkBeginCommandBuffer(getCurrentCommandBuffer(), &beginInfo), "failed to begin recording command buffer");
}

bool VulkanRenderer::endFrame() {
    checkResult(vkEndCommandBuffer(getCurrentCommandBuffer()), "failed to end command buffer");
    VkResult result = swapChain->submitCommandBuffers(&commandBuffers[swapChain->currentFrame()]);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        recreateSwapChain();
        // return true if framebuffer was resized
        return true;
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image");
    }
    return false;
}
