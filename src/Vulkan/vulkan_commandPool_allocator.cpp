#include "vulkan_device.hpp"

// TODO Improve this
VulkanDevice::VulkanCommandPoolAllocator::VulkanCommandPoolAllocator(VulkanDevice* device) : device{device} {}
VulkanDevice::VulkanCommandPoolAllocator::~VulkanCommandPoolAllocator() {
    for (auto poolItr = pools.begin(); poolItr != pools.end(); ++poolItr) {
        for (auto bufferItr = poolItr->second.second.begin(); bufferItr != poolItr->second.second.begin(); ++bufferItr) {
            vkFreeCommandBuffers(device->device(), poolItr->first, 1, &bufferItr->first);
        }
        vkDestroyCommandPool(device->device(), poolItr->first, nullptr);
    }
}

std::mutex VulkanDevice::VulkanCommandPoolAllocator::getPoolMutex;
VkCommandPool VulkanDevice::VulkanCommandPoolAllocator::getPool() {
    std::lock_guard<std::mutex> lock(getPoolMutex);
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
    VkCommandPool pool = device->createCommandPool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    std::unordered_map<VkCommandBuffer, bool> bufferMap;
    pools.insert({pool, {1, bufferMap}});
    return pool;
}

std::mutex VulkanDevice::VulkanCommandPoolAllocator::getBufferMutex;
VkCommandBuffer VulkanDevice::VulkanCommandPoolAllocator::getBuffer(VkCommandPool pool) {
    std::lock_guard<std::mutex> lock(getBufferMutex);
    for (auto poolItr = pools.begin(); poolItr != pools.end(); ++poolItr) {
        if (poolItr->first == pool) {
            for (auto bufferItr = poolItr->second.second.begin(); bufferItr != poolItr->second.second.begin(); ++bufferItr) {
                if (!bufferItr->second) {
                    bufferItr->second = 1;
                    return bufferItr->first;
                }
            }
            // if no free buffers create buffer in pool and return it
            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool = poolItr->first;
            allocInfo.commandBufferCount = 1;

            VkCommandBuffer commandBuffer;
            vkAllocateCommandBuffers(device->device(), &allocInfo, &commandBuffer);
            poolItr->second.second.insert({commandBuffer, 1});
            return commandBuffer;
        }
    }
    throw std::runtime_error("failed to find command pool");
}

void VulkanDevice::VulkanCommandPoolAllocator::releasePool(VkCommandPool pool) {
    for (auto itr = pools.begin(); itr != pools.end(); ++itr) {
        if (!itr->second.first) {
            itr->second.first = 0;
        }
    }
}
void VulkanDevice::VulkanCommandPoolAllocator::releaseBuffer(VkCommandPool pool, VkCommandBuffer buffer) {
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
