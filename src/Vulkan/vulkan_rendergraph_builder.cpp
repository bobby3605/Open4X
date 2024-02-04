#include "vulkan_pipeline.hpp"
#include "vulkan_rendergraph.hpp"
#include <cstdint>
#include <memory>
#include <vulkan/vulkan_core.h>

VulkanRenderGraph& VulkanRenderGraph::shader(std::string computePath, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ,
                                             ShaderOptions shaderOptions) {
    std::shared_ptr<VulkanRenderGraph::VulkanShader> shader =
        std::make_shared<VulkanRenderGraph::VulkanShader>(computePath, shaderOptions.specializationInfo, _device);
    shaders.push_back(shader);
    renderOps.push_back(bindPipeline(shader));
    renderOps.push_back(bindDescriptorSets(shader));
    ShaderOptions defaultOptions{};
    if (shaderOptions.pushConstantData != defaultOptions.pushConstantData) {
        // TODO
        // Only push constants if the data pointer has changed between shaders
        renderOps.push_back(pushConstants(shader, shaderOptions.pushConstantData));
    }
    renderOps.push_back(dispatch(groupCountX, groupCountY, groupCountZ));
    return *this;
}

VulkanRenderGraph& VulkanRenderGraph::shader(std::string vertPath, std::string fragPath, ShaderOptions vertOptions,
                                             ShaderOptions fragOptions, std::shared_ptr<VulkanBuffer> vertexBuffer,
                                             std::shared_ptr<VulkanBuffer> indexBuffer) {
    std::shared_ptr<VulkanRenderGraph::VulkanShader> vert =
        std::make_shared<VulkanRenderGraph::VulkanShader>(vertPath, vertOptions.specializationInfo, _device);
    std::shared_ptr<VulkanRenderGraph::VulkanShader> frag =
        std::make_shared<VulkanRenderGraph::VulkanShader>(fragPath, fragOptions.specializationInfo, _device);
    shaders.push_back(vert);
    shaders.push_back(frag);
    renderOps.push_back(startRendering());

    // Reset viewport and scissor
    // TODO
    // only do this if the swapChain extent changed
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

    renderOps.push_back(setViewportOp(viewport));

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChain->getExtent();

    renderOps.push_back(setScissorOp(scissor));

    renderOps.push_back(bindPipeline(vert));
    renderOps.push_back(bindDescriptorSets(vert));
    ShaderOptions defaultOptions{};
    if (vertOptions.pushConstantData != defaultOptions.pushConstantData) {
        // TODO
        // Only push constants if the data pointer has changed between shaders
        renderOps.push_back(pushConstants(vert, vertOptions.pushConstantData));
    }
    renderOps.push_back(bindVertexBuffer(vertexBuffer));
    if (fragOptions.pushConstantData != defaultOptions.pushConstantData) {
        // TODO
        // Only push constants if the data pointer has changed between shaders
        renderOps.push_back(pushConstants(frag, fragOptions.pushConstantData));
    }
    renderOps.push_back(bindIndexBuffer(indexBuffer));
    /*
    std::shared_ptr<VulkanRenderGraph::VulkanShader> shader =
        std::make_shared<VulkanRenderGraph::VulkanShader>(path, shaderOptions.specializationInfo);
    shaders.push_back(shader);
    renderOps.push_back(bindPipeline(shader));
    renderOps.push_back(bindDescriptorSets(shader));
    ShaderOptions defaultOptions{};
    if (shaderOptions.pushConstantData != defaultOptions.pushConstantData) {
        // TODO
        // Only push constants if the data pointer has changed between shaders
        renderOps.push_back(pushConstants(shader, shaderOptions.pushConstantData));
    }
    renderOps.push_back(dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ));
    */
    renderOps.push_back(endRendering());
    return *this;
}

VulkanRenderGraph& VulkanRenderGraph::buffer(std::string name, uint32_t count) {
    if (bufferCounts.count(name) == 0) {
        bufferCounts[name] = count;
    } else {
        throw std::runtime_error("multiple buffer definitions for: " + name);
    }
    return *this;
}

VulkanRenderGraph& VulkanRenderGraph::buffer(std::string name, std::shared_ptr<VulkanBuffer> buffer) {
    globalBuffers[name] = buffer;
    return *this;
}

VulkanRenderGraph& VulkanRenderGraph::imageInfos(std::string name, std::vector<VkDescriptorImageInfo>& imageInfos) {
    globalImageInfos[name] = &imageInfos;
    return *this;
}

VulkanRenderGraph& VulkanRenderGraph::fillBuffer(std::string bufferName, VkDeviceSize offset, VkDeviceSize size, uint32_t value) {
    if (globalBuffers.count(bufferName) == 0) {
        throw std::runtime_error("fillBuffer: buffer " + bufferName + " not found");
    }
    renderOps.push_back(fillBufferOp(globalBuffers[bufferName]->buffer(), offset, size, value));
    return *this;
}

VulkanRenderGraph& VulkanRenderGraph::setBuffer(std::string bufferName, uint32_t value) {
    if (globalBuffers.count(bufferName) == 0) {
        throw std::runtime_error("setBuffer: buffer " + bufferName + " not found");
    }
    fillBuffer(bufferName, 0, globalBuffers[bufferName]->size(), value);
    return *this;
}

// TODO auto reset
VulkanRenderGraph& VulkanRenderGraph::queryReset(VkQueryPool queryPool, uint32_t firstQuery, uint32_t count) {
    renderOps.push_back(resetQueryPool(queryPool, firstQuery, count));
    return *this;
}

VulkanRenderGraph& VulkanRenderGraph::timestamp(VkPipelineStageFlags2 stageFlags, VkQueryPool queryPool, uint32_t query) {
    renderOps.push_back(writeTimestamp(stageFlags, queryPool, query));
    return *this;
}

VulkanRenderGraph& VulkanRenderGraph::memoryBarrier(VkAccessFlags2 srcAccessMask, VkPipelineStageFlags2 srcStageMask,
                                                    VkAccessFlags2 dstAccessMask, VkPipelineStageFlags2 dstStageMask) {
    renderOps.push_back(memoryBarrierOp(srcAccessMask, srcStageMask, dstAccessMask, dstStageMask));
    return *this;
}
VulkanRenderGraph& VulkanRenderGraph::debugBarrier() {
    memoryBarrier(VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                      VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                  VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                  VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                      VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                  VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT);
    return *this;
}

VulkanRenderGraph& VulkanRenderGraph::drawIndirect(std::string buffer, VkDeviceSize offset, std::string countBuffer,
                                                   VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride) {
    renderOps.push_back(drawIndexedIndirectCount(globalBuffers[buffer]->buffer(), offset, globalBuffers[countBuffer]->buffer(),
                                                 countBufferOffset, maxDrawCount, stride));
    return *this;
}

std::vector<VkPushConstantRange> VulkanRenderGraph::getPushConstants(std::shared_ptr<VulkanRenderGraph::VulkanShader> shader) {
    // TODO
    // Multiple shaders for input
    std::vector<VkPushConstantRange> pushConstants;
    if (shader->pushConstantRange.size != 0) {
        shader->pushConstantRange.offset = pushConstantOffset;
        pushConstantOffset += shader->pushConstantRange.size;
        pushConstants.push_back(shader->pushConstantRange);
    }
    return pushConstants;
}

void VulkanRenderGraph::addComputePipeline(std::shared_ptr<VulkanRenderGraph::VulkanShader> computeShader,
                                           std::vector<VkDescriptorSetLayout>& layouts) {
    std::vector<VkPushConstantRange> pushConstants(1);
    pushConstants = getPushConstants(computeShader);
    pipelines[computeShader->name] = std::make_shared<ComputePipeline>(_device, layouts, pushConstants, computeShader->stageInfo);
}

void VulkanRenderGraph::addGraphicsPipeline(std::shared_ptr<VulkanRenderGraph::VulkanShader> vertShader,
                                            std::shared_ptr<VulkanRenderGraph::VulkanShader> fragShader) {
    std::vector<VkDescriptorSetLayout> layouts = descriptorManager->descriptors[vertShader->name]->getLayouts();
    layouts.insert(layouts.end(), descriptorManager->descriptors[fragShader->name]->getLayouts().begin(),
                   descriptorManager->descriptors[fragShader->name]->getLayouts().end());
    std::vector<VkPushConstantRange> pushConstants(2);
    // FIXME:
    // Use getPushConstants
    // Needs to support 0 offsets for both shaders
    pushConstants.push_back(vertShader->pushConstantRange);
    pushConstants.push_back(fragShader->pushConstantRange);
    pipelines[vertShader->name] =
        std::make_shared<GraphicsPipeline>(_device, swapChain, vertShader->stageInfo, fragShader->stageInfo, layouts, pushConstants);
}

void VulkanRenderGraph::compile() {
    std::shared_ptr<VulkanShader> prev;
    for (auto shader : shaders) {
        std::cout << "compiling shader: " << shader->name << std::endl;

        shader->compile();
        shader->createShaderModule();

        // TODO
        // Share buffers between stages with the same descriptor
        // For example, culling compute shaders re-use buffers between stages
        // These re-used buffers only need to be bound once
        // Total set bindings might have to be identical though, so maybe this won't work
        VulkanDescriptors::VulkanDescriptor* descriptor = descriptorManager->createDescriptor(shader->name, shader->stageFlags);
        std::cout << "setting descriptor buffers" << shader->name << std::endl;
        shader->setDescriptorBuffers(descriptor, bufferCounts, globalBuffers, globalImageInfos);

        std::cout << "switching flags: " << shader->stageFlags << std::endl;
        switch (shader->stageFlags) {
        case VK_SHADER_STAGE_COMPUTE_BIT: {
            std::vector<VkDescriptorSetLayout> layouts = descriptor->getLayouts();
            addComputePipeline(shader, layouts);
            break;
        }
        case VK_SHADER_STAGE_VERTEX_BIT: {
            break;
        }
        case VK_SHADER_STAGE_FRAGMENT_BIT:
            // NOTE:
            // fragment always comes after vertex
            // FIXME:
            // find a better way to do this
            addGraphicsPipeline(prev, shader);
            break;
        default:
            throw std::runtime_error("Unknown shader type type during rendergraph compilation: " + std::to_string(shader->stageFlags));
        }
        prev = shader;
    }
    std::cout << "compiled shaders" << std::endl;
}
