#include "command_runner.hpp"
#include "device.hpp"
#include <vulkan/vulkan_core.h>

VkCommandPool CommandRunner::_pool = VK_NULL_HANDLE;
CommandRunner::CommandRunner() {
    if (_pool == VK_NULL_HANDLE) {
        _pool = Device::device->command_pools()->get_pool();
    }

    _command_buffer = Device::device->command_pools()->get_primary(_pool);
    begin_recording();
}

void CommandRunner::begin_recording() {
    VkCommandBufferBeginInfo begin_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(_command_buffer, &begin_info);
}

void CommandRunner::end_recording() {
    vkEndCommandBuffer(_command_buffer);

    std::vector<VkSubmitInfo> submit_infos;
    VkSubmitInfo submit_info{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &_command_buffer;

    submit_infos.push_back(submit_info);

    Device::device->submit_queue(VK_QUEUE_TRANSFER_BIT, submit_infos);

    vkResetCommandBuffer(_command_buffer, 0);
}

void CommandRunner::run() {
    end_recording();
    Device::device->command_pools()->release_buffer(_pool, _command_buffer);
    //    delete this;
}

void CommandRunner::copy_buffer(VkBuffer dst, VkBuffer src, std::vector<VkBufferCopy>& copy_infos) {
    vkCmdCopyBuffer(_command_buffer, src, dst, copy_infos.size(), copy_infos.data());
}

void CommandRunner::copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(_command_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

std::pair<std::shared_ptr<VkImageMemoryBarrier2>, std::shared_ptr<VkDependencyInfo>>
CommandRunner::transition_image(VkImageLayout old_layout, VkImageLayout new_layout, uint32_t mip_levels) {
    return transition_image(old_layout, new_layout, VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, mip_levels, 0, 1});
}
std::pair<std::shared_ptr<VkImageMemoryBarrier2>, std::shared_ptr<VkDependencyInfo>>
CommandRunner::transition_image(VkImageLayout old_layout, VkImageLayout new_layout, VkImageSubresourceRange subresource_range) {
    VkImageMemoryBarrier2 barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    // NOTE:
    // Set in the pointers returned
    //    barrier.image = image;

    barrier.subresourceRange = subresource_range;

    return image_barrier(barrier);
}

std::pair<std::shared_ptr<VkImageMemoryBarrier2>, std::shared_ptr<VkDependencyInfo>>
CommandRunner::image_barrier(VkImageMemoryBarrier2& barrier) {
    VkImageLayout& old_layout = barrier.oldLayout;
    VkImageLayout& new_layout = barrier.newLayout;

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_2_NONE;
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

        barrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

    } else if (old_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
        barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

        barrier.dstAccessMask = VK_ACCESS_2_NONE;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;

        barrier.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_2_NONE;
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;

        barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;

        barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    } else {
        throw std::invalid_argument("unsupported layout transition");
    }

    auto tmp_barrier = std::make_shared<VkImageMemoryBarrier2>(barrier);
    auto dependency_info = std::make_shared<VkDependencyInfo>();

    dependency_info->sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependency_info->dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    dependency_info->imageMemoryBarrierCount = 1;
    dependency_info->pImageMemoryBarriers = tmp_barrier.get();

    return {tmp_barrier, dependency_info};
}
