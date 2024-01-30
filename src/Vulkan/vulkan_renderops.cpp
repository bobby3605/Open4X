#include "vulkan_descriptors.hpp"
#include "vulkan_rendergraph.hpp"
#include <algorithm>
#include <memory>
#include <vulkan/vulkan_core.h>

VulkanRenderGraph::RenderOp VulkanRenderGraph::bindPipeline(std::shared_ptr<VulkanRenderGraph::VulkanShader> shader) {
    return [&](VkCommandBuffer commandBuffer) { vkCmdBindPipeline(commandBuffer, shader->bindPoint, pipelines[shader->name]->pipeline()); };
}

VulkanRenderGraph::RenderOp VulkanRenderGraph::bindDescriptorSets(std::shared_ptr<VulkanShader> shader) {
    return [&](VkCommandBuffer commandBuffer) {
        // NOTE:
        // Have to bind one at a time because descriptor sets might not be sequential
        // TODO
        // Check if binding sequential sets is faster. It probably isn't due to the low amount of sets
        for (auto set : descriptorManager->descriptors[shader->name]->getSets()) {
            vkCmdBindDescriptorSets(getCurrentCommandBuffer(), shader->bindPoint, pipelines[shader->name]->pipelineLayout(), set.first, 1,
                                    &set.second, 0, nullptr);
        }
    };
}

// FIXME:
// reorder pushConstants to happen between the bind and dispatch/draw
// TODO
// Generate function pointer for type, so size no longer needs to be passed
// Look at GLTF::AccessorLoader for example
VulkanRenderGraph::RenderOp VulkanRenderGraph::pushConstants(std::shared_ptr<VulkanShader> shader, uint32_t size, void* data,
                                                             uint32_t offset) {
    return [&](VkCommandBuffer commandBuffer) {
        vkCmdPushConstants(getCurrentCommandBuffer(), pipelines[shader->name]->pipelineLayout(), shader->stageFlags, offset, size, data);
    };
}

void VulkanRenderGraph::recordCommandBuffer(VkCommandBuffer commandBuffer) {
    for (RenderOp renderOp : renderOps) {
        renderOp(commandBuffer);
    }
}
