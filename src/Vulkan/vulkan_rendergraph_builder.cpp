#include "common.hpp"
#include "vulkan_pipeline.hpp"
#include "vulkan_rendergraph.hpp"
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

VulkanRenderGraph& VulkanRenderGraph::shader(std::string computePath, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ,
                                             ShaderOptions shaderOptions) {
    std::shared_ptr<VulkanRenderGraph::VulkanShader> shader = std::make_shared<VulkanRenderGraph::VulkanShader>(computePath, _device);
    shaders.push_back(shader);
    renderOps.push_back(bindPipeline(shader));
    renderOps.push_back(bindDescriptorSets(shader));
    ShaderOptions defaultOptions{};
    if (shaderOptions.pushConstantData != defaultOptions.pushConstantData) {
        // TODO
        // Only push constants if the data pointer has changed between shaders
        renderOps.push_back(pushConstants(shader, shaderOptions.pushConstantData));
    }
    shader->specInfo.pData = shaderOptions.specData;
    renderOps.push_back(dispatch(groupCountX, groupCountY, groupCountZ));
    return *this;
}

VulkanRenderGraph& VulkanRenderGraph::shader(std::string vertPath, std::string fragPath, ShaderOptions vertOptions,
                                             ShaderOptions fragOptions, std::shared_ptr<VulkanBuffer> vertexBuffer,
                                             std::shared_ptr<VulkanBuffer> indexBuffer) {
    std::shared_ptr<VulkanRenderGraph::VulkanShader> vert = std::make_shared<VulkanRenderGraph::VulkanShader>(vertPath, _device);
    std::shared_ptr<VulkanRenderGraph::VulkanShader> frag = std::make_shared<VulkanRenderGraph::VulkanShader>(fragPath, _device);
    shaders.push_back(vert);
    shaders.push_back(frag);
    renderOps.push_back(startRendering());

    renderOps.push_back(bindPipeline(vert));
    // FIXME:
    // Find better way to name descriptor sets,
    // possibly by pipeline name
    renderOps.push_back(bindDescriptorSets(vert));
    renderOps.push_back(bindDescriptorSets(frag));
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
    if (vert->hasSpecConstants) {
        vert->specInfo.pData = vertOptions.specData;
    }
    if (frag->hasSpecConstants) {
        frag->specInfo.pData = fragOptions.specData;
    }
    renderOps.push_back(bindIndexBuffer(indexBuffer));
    return *this;
}

VulkanRenderGraph& VulkanRenderGraph::buffer(std::string name, uint32_t count) {
    buffer(name, count, 0, 0);
    return *this;
}

VulkanRenderGraph& VulkanRenderGraph::buffer(std::string name, uint32_t count, VkBufferUsageFlags additionalUsage,
                                             VkMemoryPropertyFlags additionalProperties) {
    if (bufferCreateInfos.count(name) == 0) {
        bufferCreateInfos[name] = {count, additionalUsage, additionalProperties};
    } else {
        throw std::runtime_error("multiple buffer definitions for: " + name);
    }
    return *this;
}

VulkanRenderGraph& VulkanRenderGraph::buffer(std::string name, std::shared_ptr<VulkanBuffer> buffer) {
    globalBuffers[name] = buffer;
    return *this;
}

VulkanRenderGraph& VulkanRenderGraph::imageInfos(std::string name, std::vector<VkDescriptorImageInfo>* imageInfos) {
    globalImageInfos[name] = imageInfos;
    return *this;
}

VulkanRenderGraph& VulkanRenderGraph::fillBuffer(std::string bufferName, VkDeviceSize offset, VkDeviceSize size, uint32_t value) {
    if (globalBuffers.count(bufferName) == 0 && bufferCreateInfos.count(bufferName) == 0) {
        throw std::runtime_error("fillBuffer: buffer " + bufferName + " not found");
    }
    renderOps.push_back(fillBufferOp(bufferName, offset, size, value));
    return *this;
}

VulkanRenderGraph& VulkanRenderGraph::setBuffer(std::string bufferName, uint32_t value) {
    fillBuffer(bufferName, 0, VK_WHOLE_SIZE, value);
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
    renderOps.push_back(drawIndexedIndirectCount(buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride));
    // FIXME:
    // Find a better way to manage start and end rendering
    renderOps.push_back(endRendering());
    return *this;
}

std::vector<VkPushConstantRange> VulkanRenderGraph::getPushConstants(std::shared_ptr<VulkanRenderGraph::VulkanShader> shader) {
    // NOTE:
    // Right now this function is basically useless,
    // it's here for the future when multiple sets of push constants in a pipeline are needed
    // TODO
    // Multiple shaders for input
    std::vector<VkPushConstantRange> pushConstants;
    if (shader->pushConstantRange.size != 0) {
        // FIXME:
        // only use offset when there's multiple input shaders
        // shader->pushConstantRange.offset = pushConstantOffset;
        //        pushConstantOffset += shader->pushConstantRange.size;
        pushConstants.push_back(shader->pushConstantRange);
    }
    return pushConstants;
}

void VulkanRenderGraph::addComputePipeline(std::shared_ptr<VulkanRenderGraph::VulkanShader> computeShader,
                                           std::vector<VkDescriptorSetLayout>& layouts) {
    std::vector<VkPushConstantRange> pushConstants(1);
    pushConstants = getPushConstants(computeShader);
    pipelines[getFilenameNoExt(computeShader->name)] =
        std::make_shared<ComputePipeline>(_device, layouts, pushConstants, computeShader->stageInfo);
}

void VulkanRenderGraph::addGraphicsPipeline(std::shared_ptr<VulkanRenderGraph::VulkanShader> vertShader,
                                            std::shared_ptr<VulkanRenderGraph::VulkanShader> fragShader) {
    if (vertShader->stageFlags != VK_SHADER_STAGE_VERTEX_BIT) {
        throw std::runtime_error("graphics pipeline vertShader has incorrect stageFlags: " + std::to_string(vertShader->stageFlags));
    }
    if (fragShader->stageFlags != VK_SHADER_STAGE_FRAGMENT_BIT) {
        throw std::runtime_error("graphics pipeline fragShader has incorrect stageFlags: " + std::to_string(fragShader->stageFlags));
    }
    // FIXME:
    // Better handling of names for descriptors
    std::vector<VkDescriptorSetLayout> vertLayouts = descriptorManager->descriptors[vertShader->path]->getLayouts();
    std::vector<VkDescriptorSetLayout> fragLayouts = descriptorManager->descriptors[fragShader->path]->getLayouts();
    std::vector<VkDescriptorSetLayout> layouts;
    // FIXME:
    // maybe have to be in set order???
    // so set 0, then 1, then 2
    // set 0 and 2 are in vert
    // set 1 is in frag
    // can probably easily swap given new interface
    for (auto layout : vertLayouts) {
        layouts.push_back(layout);
    }
    for (auto layout : fragLayouts) {
        layouts.push_back(layout);
    }
    std::vector<VkPushConstantRange> pushConstants;
    // FIXME:
    // Use getPushConstants
    // Needs to support 0 offsets for both shaders
    if (vertShader->hasPushConstants) {
        pushConstants.push_back(vertShader->pushConstantRange);
    }
    if (fragShader->hasPushConstants) {
        pushConstants.push_back(fragShader->pushConstantRange);
    }
    pipelines[getFilenameNoExt(vertShader->name)] =
        std::make_shared<OldGraphicsPipeline>(_device, swapChain, vertShader->stageInfo, fragShader->stageInfo, layouts, pushConstants);
}

void VulkanRenderGraph::compile() {
    std::shared_ptr<VulkanShader> prev;
    for (auto shader : shaders) {

        shader->compile();
        shader->createShaderModule();

        // TODO
        // Share buffers between stages with the same descriptor
        // For example, culling compute shaders re-use buffers between stages
        // These re-used buffers only need to be bound once
        // Total set bindings might have to be identical though, so maybe this won't work
        VulkanDescriptors::VulkanDescriptor* descriptor = descriptorManager->createDescriptor(shader->path, shader->stageFlags);
        shader->setDescriptorBuffers(descriptor, bufferCreateInfos, globalBuffers, globalImageInfos);

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
}
