#include "vulkan_rendergraph.hpp"
#include "Include/BaseTypes.h"
#include "vulkan_buffer.hpp"
#include "vulkan_descriptors.hpp"
#include "vulkan_pipeline.hpp"
#include <memory>
#include <stdexcept>
#include <string>
#include <vulkan/vulkan_core.h>

static VkDescriptorType switchStorageType(glslang::TStorageQualifier storageType) {
    switch (storageType) {
    case glslang::EvqUniform:
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    case glslang::EvqBuffer:
        return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    default:
        throw std::runtime_error("switchStorageType unsupported type: " + std::to_string(storageType));
    }
}

VulkanRenderGraph::VulkanRenderGraph(std::shared_ptr<VulkanDevice> device) {
    _device = device;
    descriptorManager = std::make_shared<VulkanDescriptors>(device);
}

VulkanRenderGraph& VulkanRenderGraph::shader(std::string path, VkSpecializationInfo* specInfo) {
    std::shared_ptr<VulkanRenderGraph::VulkanShader> shader = std::make_shared<VulkanRenderGraph::VulkanShader>(path, specInfo);
    shaders.push_back(shader);
    renderOps.push_back(bindPipeline(shader));
    renderOps.push_back(bindDescriptorSets(shader));
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
    pipelines[computeShader->name] =
        std::make_shared<ComputePipeline>(_device, layouts, getPushConstants(computeShader), computeShader->stageInfo);
}

void VulkanRenderGraph::compile() {
    for (auto shader : shaders) {

        shader->compile();
        shader->createShaderModule();

        VulkanDescriptors::VulkanDescriptor* descriptor = descriptorManager->createDescriptor(shader->name, shader->stageFlags);
        shader->setDescriptorBuffers(descriptor, bufferCounts, globalBuffers);

        switch (shader->stageFlags) {
        case VK_SHADER_STAGE_COMPUTE_BIT: {
            std::vector<VkDescriptorSetLayout> layouts = descriptor->getLayouts();
            addComputePipeline(shader, layouts);
            break;
        }
            /*
        case VK_SHADER_STAGE_VERTEX_BIT:
            addGraphicsPipeline(shader, std::shared_ptr<VulkanShader> fragShader) break;
            */
        default:
            throw std::runtime_error("Unknown shader type type during rendergraph compilation: " + std::to_string(shader->stageFlags));
        }
    }
}
