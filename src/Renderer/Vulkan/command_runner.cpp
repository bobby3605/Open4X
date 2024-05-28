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
