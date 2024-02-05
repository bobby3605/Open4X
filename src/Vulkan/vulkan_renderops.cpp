#include "common.hpp"
#include "vulkan_descriptors.hpp"
#include "vulkan_rendergraph.hpp"
#include <algorithm>
#include <cstdint>
#include <memory>
#include <vulkan/vulkan_core.h>

RenderOp VulkanRenderGraph::bindPipeline(std::shared_ptr<VulkanRenderGraph::VulkanShader> shader) {
    return [&, shader](VkCommandBuffer commandBuffer) {
        vkCmdBindPipeline(commandBuffer, shader->bindPoint, pipelines[getFilenameNoExt(shader->name)]->pipeline());
    };
}

RenderOp VulkanRenderGraph::bindDescriptorSets(std::shared_ptr<VulkanShader> shader) {
    return [&, shader](VkCommandBuffer commandBuffer) {
        // NOTE:
        // Have to bind one at a time because descriptor sets might not be sequential
        // TODO
        // Check if binding sequential sets is faster. It probably isn't due to the low amount of sets
        // NOTE:
        // Does shader->path, shader->bindPoint, etc get called at lambda creation time or lambda run time?
        // Might be undefined behavior, compiler could optimize it for creation time and break this.
        // Default seems to be run time.
        // https://stackoverflow.com/a/37221299
        // https://johannesugb.github.io/cpu-programming/how-to-pass-lambda-functions-in-C++/
        for (auto set : descriptorManager->descriptors[shader->path]->getSets()) {
            // FIXME:
            // Better pipeline naming
            vkCmdBindDescriptorSets(commandBuffer, shader->bindPoint, pipelines[getFilenameNoExt(shader->name)]->pipelineLayout(),
                                    set.first, 1, &set.second, 0, nullptr);
        }
    };
}

RenderOp VulkanRenderGraph::startRendering() {
    return [&](VkCommandBuffer commandBuffer) {
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
        _device->transitionImageLayout(swapChain->getSwapChainImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                       VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1})(commandBuffer);

        _device->transitionImageLayout(swapChain->getDepthImage(), VK_IMAGE_LAYOUT_UNDEFINED,
                                       VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                       VkImageSubresourceRange{VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1})(commandBuffer);

        vkCmdBeginRendering(commandBuffer, &passInfo);
    };
}

RenderOp VulkanRenderGraph::endRendering() {
    return [=](VkCommandBuffer commandBuffer) {
        vkCmdEndRendering(commandBuffer);

        _device->transitionImageLayout(swapChain->getSwapChainImage(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                       VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                       VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1})(commandBuffer);
    };
}

RenderOp VulkanRenderGraph::pushConstants(std::shared_ptr<VulkanShader> shader, void* data) {
    return [=](VkCommandBuffer commandBuffer) {
        vkCmdPushConstants(commandBuffer, pipelines[getFilenameNoExt(shader->name)]->pipelineLayout(), shader->stageFlags,
                           shader->pushConstantRange.offset, shader->pushConstantRange.size, data);
        /*
        std::cout << "uploading push constant data[0]: " << reinterpret_cast<uint32_t*>(data)[0]
                  << " size: " << shader->pushConstantRange.size << " offset: " << shader->pushConstantRange.offset << std::endl;
                  */
    };
}

RenderOp VulkanRenderGraph::fillBufferOp(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, uint32_t value) {
    return [=](VkCommandBuffer commandBuffer) { vkCmdFillBuffer(commandBuffer, buffer, offset, size, value); };
}

RenderOp VulkanRenderGraph::dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
    return [=](VkCommandBuffer commandBuffer) { vkCmdDispatch(commandBuffer, groupCountX, groupCountY, groupCountZ); };
}

RenderOp VulkanRenderGraph::drawIndexedIndirectCount(VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer,
                                                     VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride) {
    return [=](VkCommandBuffer commandBuffer) {
        vkCmdDrawIndexedIndirectCount(commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
    };
}

RenderOp VulkanRenderGraph::writeTimestamp(VkPipelineStageFlags2 stageFlags, VkQueryPool queryPool, uint32_t query) {
    return [=](VkCommandBuffer commandBuffer) { vkCmdWriteTimestamp2(commandBuffer, stageFlags, queryPool, query); };
}

RenderOp VulkanRenderGraph::resetQueryPool(VkQueryPool queryPool, uint32_t firstQuery, uint32_t count) {
    return [=](VkCommandBuffer commandBuffer) { vkCmdResetQueryPool(commandBuffer, queryPool, firstQuery, count); };
}

RenderOp VulkanRenderGraph::bindVertexBuffer(std::shared_ptr<VulkanBuffer> vertexBuffer) {
    VkBuffer vertexBuffers[] = {vertexBuffer->buffer()};
    VkDeviceSize offsets[] = {0};
    return [=](VkCommandBuffer commandBuffer) { vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets); };
}

RenderOp VulkanRenderGraph::bindIndexBuffer(std::shared_ptr<VulkanBuffer> indexBuffer) {
    return [=](VkCommandBuffer commandBuffer) { vkCmdBindIndexBuffer(commandBuffer, indexBuffer->buffer(), 0, VK_INDEX_TYPE_UINT32); };
}

RenderOp VulkanRenderGraph::memoryBarrierOp(VkAccessFlags2 srcAccessMask, VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 dstAccessMask,
                                            VkPipelineStageFlags2 dstStageMask) {
    return [=](VkCommandBuffer commandBuffer) {
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
    };
}

RenderOp VulkanRenderGraph::setViewportOp(VkViewport viewport) {
    return [=](VkCommandBuffer commandBuffer) { vkCmdSetViewport(commandBuffer, 0, 1, &viewport); };
}

RenderOp VulkanRenderGraph::setScissorOp(VkRect2D scissor) {
    return [=](VkCommandBuffer commandBuffer) { vkCmdSetScissor(commandBuffer, 0, 1, &scissor); };
}

void VulkanRenderGraph::recordRenderOps(VkCommandBuffer commandBuffer) {
    for (RenderOp renderOp : renderOps) {
        renderOp(commandBuffer);
    }
}
