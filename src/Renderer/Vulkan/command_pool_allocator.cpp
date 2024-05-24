#include "common.hpp"
#include "device.hpp"
#include <vulkan/vulkan_core.h>

// TODO Improve this
Device::CommandPoolAllocator::CommandPoolAllocator() {}
Device::CommandPoolAllocator::~CommandPoolAllocator() {
    for (auto poolItr = pools.begin(); poolItr != pools.end(); ++poolItr) {
        for (auto bufferItr = poolItr->second.second.begin(); bufferItr != poolItr->second.second.begin(); ++bufferItr) {
            vkFreeCommandBuffers(Device::device->vk_device(), poolItr->first, 1, &bufferItr->first);
        }
        vkDestroyCommandPool(Device::device->vk_device(), poolItr->first, nullptr);
    }
}

std::mutex Device::CommandPoolAllocator::get_pool_mutex;
VkCommandPool Device::CommandPoolAllocator::get_pool() {
    std::lock_guard<std::mutex> lock(get_pool_mutex);
    // Iterate over all pools
    for (auto itr = pools.begin(); itr != pools.end(); ++itr) {
        // If the pool is not allocated
        if (!itr->second.first) {
            // mark pool as allocated and return it
            itr->second.first = 1;
            return itr->first;
        }
    }
    // if no free pools, create one and return it
    // TODO:
    // Add a flag parameter and return a pool with the correct flag
    // this would also mean adding support for transient pools
    VkCommandPool pool = device->create_command_pool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    std::unordered_map<VkCommandBuffer, bool> bufferMap;
    pools.insert({pool, {1, bufferMap}});
    return pool;
}

std::mutex Device::CommandPoolAllocator::get_buffer_mutex;
VkCommandBuffer Device::CommandPoolAllocator::get_buffer(VkCommandPool pool, VkCommandBufferLevel level) {
    std::lock_guard<std::mutex> lock(get_buffer_mutex);
    for (auto poolItr = pools.begin(); poolItr != pools.end(); ++poolItr) {
        if (poolItr->first == pool) {
            for (auto bufferItr = poolItr->second.second.begin(); bufferItr != poolItr->second.second.begin(); ++bufferItr) {
                if (!bufferItr->second) {
                    bufferItr->second = 1;
                    return bufferItr->first;
                }
            }
            // if no free buffers create buffer in pool and return it
            VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
            allocInfo.level = level;
            allocInfo.commandPool = poolItr->first;
            allocInfo.commandBufferCount = 1;

            VkCommandBuffer commandBuffer;
            check_result(vkAllocateCommandBuffers(Device::device->vk_device(), &allocInfo, &commandBuffer),
                         "failed to create command buffer!");
            poolItr->second.second.insert({commandBuffer, 1});
            return commandBuffer;
        }
    }
    throw std::runtime_error("failed to find command pool");
}

VkCommandBuffer Device::CommandPoolAllocator::get_primary(VkCommandPool pool) { return get_buffer(pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY); }
VkCommandBuffer Device::CommandPoolAllocator::get_secondary(VkCommandPool pool) {
    return get_buffer(pool, VK_COMMAND_BUFFER_LEVEL_SECONDARY);
}

void Device::CommandPoolAllocator::release_pool(VkCommandPool pool) {
    for (auto itr = pools.begin(); itr != pools.end(); ++itr) {
        if (!itr->second.first) {
            itr->second.first = 0;
        }
    }
}
void Device::CommandPoolAllocator::release_buffer(VkCommandPool pool, VkCommandBuffer buffer) {
    for (auto poolItr = pools.begin(); poolItr != pools.end(); ++poolItr) {
        if (poolItr->first == pool) {
            for (auto bufferItr = poolItr->second.second.begin(); bufferItr != poolItr->second.second.begin(); ++bufferItr) {
                if (!bufferItr->second) {
                    bufferItr->second = 0;
                }
            }
        }
    }
}
