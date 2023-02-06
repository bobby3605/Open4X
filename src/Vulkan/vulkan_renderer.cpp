#include "vulkan_renderer.hpp"
#include "vulkan_buffer.hpp"
#include "vulkan_descriptors.hpp"
#include "vulkan_device.hpp"
#include <cstdint>
#include <memory>
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

VulkanRenderer::VulkanRenderer(VulkanWindow* window, VulkanDevice* deviceRef, VulkanDescriptors* descriptorManager,
                               const std::vector<VkDrawIndexedIndirectCommand>& drawCommands)
    : vulkanWindow{window}, device{deviceRef}, descriptorManager{descriptorManager} {
    init(drawCommands);
}

VulkanRenderer::~VulkanRenderer() {
    delete graphicsPipeline;
    vkDestroyPipelineLayout(device->device(), pipelineLayout, nullptr);
    delete swapChain;
}

void VulkanRenderer::init(const std::vector<VkDrawIndexedIndirectCommand>& drawCommands) {
    swapChain = new VulkanSwapChain(device, vulkanWindow->getExtent());
    createCommandBuffers();
    createCullingPipelines(drawCommands);
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

    vkCmdBindPipeline(getCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline->pipeline());
}

void VulkanRenderer::bindComputePipeline(std::string name) {
    vkCmdBindPipeline(getCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_COMPUTE, computePipelines[name]->pipeline());
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

// TODO
// decouple objects from renderer
void VulkanRenderer::createCullingPipelines(const std::vector<VkDrawIndexedIndirectCommand>& drawCommands) {

    struct SpecData {
        uint32_t local_size_x;
        uint32_t subgroup_size;
    } specData;

    std::vector<VkSpecializationMapEntry> specEntries(2);
    specEntries[0].constantID = 0;
    specEntries[0].offset = offsetof(SpecData, local_size_x);
    specEntries[0].size = sizeof(SpecData::local_size_x);
    specEntries[1].constantID = 1;
    specEntries[1].offset = offsetof(SpecData, subgroup_size);
    specEntries[1].size = sizeof(SpecData::subgroup_size);

    VkSpecializationInfo specInfo;
    specInfo.mapEntryCount = specEntries.size();
    specInfo.pMapEntries = specEntries.data();
    specInfo.dataSize = sizeof(specData);
    specInfo.pData = &specData;

    std::vector<VkPushConstantRange> pushConstants(1);
    std::vector<VkDescriptorSetLayout> descriptorLayouts(1);

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushConstantRange.offset = 0;
    pushConstants[0] = pushConstantRange;

    std::string name;
    VulkanDescriptors::VulkanDescriptor* descriptor;

    specData.local_size_x = device->maxComputeWorkGroupInvocations();
    specData.subgroup_size = device->maxSubgroupSize();

    name = "cull_frustum_pass";
    descriptor = descriptorManager->descriptors[name];
    descriptor->allocateSets();
    descriptor->update();

    descriptorLayouts[0] = descriptor->getLayout();

    pushConstants[0].size = sizeof(ComputePushConstants);
    createComputePipeline(name, descriptorLayouts, pushConstants, &specInfo);

    name = "reduce_prefix_sum";
    descriptor = descriptorManager->descriptors[name];
    descriptor->allocateSets();
    descriptor->update();
    descriptorLayouts[0] = descriptor->getLayout();
    createComputePipeline(name, descriptorLayouts, pushConstants, &specInfo);

    name = "cull_draw_pass";
    descriptor = descriptorManager->descriptors[name];
    culledDrawIndirectCount = std::make_shared<VulkanBuffer>(device, sizeof(uint32_t),
                                                             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                                                 VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
                                                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    descriptor->addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, culledDrawIndirectCount->buffer);

    culledDrawCommandsBuffer = std::make_shared<VulkanBuffer>(device, sizeof(drawCommands[0]) * drawCommands.size(),
                                                              VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    descriptor->addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, culledDrawCommandsBuffer->buffer);

    descriptor->allocateSets();
    descriptor->update();

    descriptorLayouts[0] = descriptor->getLayout();

    pushConstants[0].size = sizeof(uint32_t);
    createComputePipeline(name, descriptorLayouts, pushConstants, &specInfo);
}

void VulkanRenderer::createComputePipeline(std::string name, std::vector<VkDescriptorSetLayout>& descriptorLayouts,
                                           std::vector<VkPushConstantRange>& pushConstants, VkSpecializationInfo* specializationInfo) {

    computePipelines[name] = std::make_shared<VulkanPipeline>(device, "build/assets/shaders/" + name + ".comp.spv", descriptorLayouts,
                                                              pushConstants, specializationInfo);
}

void VulkanRenderer::createPipeline() {
    /*
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstants);
    */

    std::vector<VkDescriptorSetLayout> descriptorLayouts;
    descriptorLayouts.push_back(descriptorManager->descriptors["global"]->getLayout());
    descriptorLayouts.push_back(descriptorManager->descriptors["material"]->getLayout());
    descriptorLayouts.push_back(descriptorManager->descriptors["object"]->getLayout());

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = descriptorLayouts.size();
    pipelineLayoutInfo.pSetLayouts = descriptorLayouts.data();
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

void VulkanRenderer::memoryBarrier(VkAccessFlags2 srcAccessMask, VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 dstAccessMask,
                                   VkPipelineStageFlags2 dstStageMask) {
    VkMemoryBarrier2 memoryBarrier{};
    memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
    memoryBarrier.pNext = VK_NULL_HANDLE;
    memoryBarrier.srcAccessMask = srcAccessMask;
    memoryBarrier.srcStageMask = srcStageMask;
    memoryBarrier.dstAccessMask = dstAccessMask;
    memoryBarrier.dstStageMask = dstStageMask;

    VkDependencyInfo dependencyInfo{};
    dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependencyInfo.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    dependencyInfo.memoryBarrierCount = 1;
    dependencyInfo.pMemoryBarriers = &memoryBarrier;

    vkCmdPipelineBarrier2(getCurrentCommandBuffer(), &dependencyInfo);
}

void VulkanRenderer::cullDraws(const std::vector<VkDrawIndexedIndirectCommand>& drawCommands,
                               ComputePushConstants& frustumCullPushConstants) {
    std::string name;

    name = "cull_frustum_pass";

    bindComputePipeline(name);

    // bind descriptors for cull_frustum_pass
    bindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE, computePipelines[name]->pipelineLayout(), 0,
                      descriptorManager->descriptors[name]->getSets()[0]);

    vkCmdPushConstants(getCurrentCommandBuffer(), computePipelines[name]->pipelineLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0,
                       sizeof(frustumCullPushConstants), &frustumCullPushConstants);

    // frustum culling
    vkCmdDispatch(getCurrentCommandBuffer(),
                  getGroupCount(frustumCullPushConstants.totalInstanceCount, device->maxComputeWorkGroupInvocations()), 1, 1);

    // wait until the frustum culling is done
    memoryBarrier(VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
                  VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);

    // reduce prefix sum step
    name = "reduce_prefix_sum";

    bindComputePipeline(name);

    // bind descriptors for reduce_prefix_sum
    bindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE, computePipelines[name]->pipelineLayout(), 0,
                      descriptorManager->descriptors[name]->getSets()[0]);

    vkCmdPushConstants(getCurrentCommandBuffer(), computePipelines[name]->pipelineLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0,
                       sizeof(frustumCullPushConstants), &frustumCullPushConstants);

    // reduce_prefix_sum
    vkCmdDispatch(getCurrentCommandBuffer(),
                  getGroupCount(frustumCullPushConstants.totalInstanceCount, device->maxComputeWorkGroupInvocations()), 1, 1);

    // wait until finished
    memoryBarrier(VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
                  VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);

    name = "cull_draw_pass";

    // bind the cull draw pipeline
    bindComputePipeline(name);

    // zero out scratch buffers
    vkCmdFillBuffer(getCurrentCommandBuffer(), culledDrawIndirectCount->buffer, 0, culledDrawIndirectCount->size(), 0);

    // barrier until the buffer has been cleared
    memoryBarrier(VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_COPY_BIT,
                  VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);

    // bind descriptors for cull draw pass
    bindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE, computePipelines[name]->pipelineLayout(), 0,
                      descriptorManager->descriptors[name]->getSets()[0]);

    // push the draw indirect count
    drawCount = drawCommands.size();
    vkCmdPushConstants(getCurrentCommandBuffer(), computePipelines[name]->pipelineLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0,
                       sizeof(uint32_t), &drawCount);

    // cull draws
    vkCmdDispatch(getCurrentCommandBuffer(), getGroupCount(drawCommands.size(), device->maxComputeWorkGroupInvocations()), 1, 1);

    // wait until culling is completed
    memoryBarrier(VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                  VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_READ_BIT, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT);
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
