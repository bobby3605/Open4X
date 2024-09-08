#include "command_runner.hpp"
#include "device.hpp"
#include <iostream>
#include <vulkan/vulkan_core.h>

VkCommandPool CommandRunner::_pool = VK_NULL_HANDLE;
CommandRunner::CommandRunner() {
    if (_pool == VK_NULL_HANDLE) {
        _pool = Device::device->command_pools()->get_pool();
    }

    _command_buffer = Device::device->command_pools()->get_primary(_pool);
    begin_recording();
}
CommandRunner::~CommandRunner() {
    if (!_did_run) {
        std::cout << "warning: did not run command runner" << std::endl;
    }
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
    _did_run = true;
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

void CommandRunner::transition_image(VkImageLayout old_layout, VkImageLayout new_layout, uint32_t mip_levels, VkImage image) {
    transition_image(old_layout, new_layout, VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, mip_levels, 0, 1}, image);
}

void CommandRunner::transition_image(VkImageLayout old_layout, VkImageLayout new_layout, VkImageSubresourceRange subresource_range,
                                     VkImage image) {
    std::shared_ptr<VkImageMemoryBarrier2> tmp_barrier;
    std::shared_ptr<VkDependencyInfo> dependency_info;
    _dependencies.push_back(tmp_barrier);
    _dependencies.push_back(dependency_info);
    std::tie(tmp_barrier, dependency_info) = transition_image(old_layout, new_layout, subresource_range);
    tmp_barrier->image = image;
    vkCmdPipelineBarrier2(_command_buffer, dependency_info.get());
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

void CommandRunner::generate_mipmaps(VkImage image, VkFormat image_format, int32_t tex_width, int32_t tex_height, uint32_t mip_levels) {

    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(Device::device->physical_device(), image_format, &formatProperties);

    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        throw std::runtime_error("texture image format does not support linear blitting");
    }

    VkImageMemoryBarrier2 barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    VkDependencyInfo dependency_info{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependency_info.imageMemoryBarrierCount = 1;
    dependency_info.pImageMemoryBarriers = &barrier;

    int32_t mip_width = tex_width;
    int32_t mip_height = tex_height;

    for (uint32_t i = 1; i < mip_levels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier2(_command_buffer, &dependency_info);

        VkImageBlit blit{};
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {mip_width, mip_height, 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = {mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, 1};
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(_command_buffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit,
                       VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;

        vkCmdPipelineBarrier2(_command_buffer, &dependency_info);

        if (mip_width > 1)
            mip_width /= 2;
        if (mip_height > 1)
            mip_height /= 2;
    }

    barrier.subresourceRange.baseMipLevel = mip_levels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;

    vkCmdPipelineBarrier2(_command_buffer, &dependency_info);
}
