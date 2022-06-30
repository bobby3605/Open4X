#ifndef VULKAN_BUFFER_H_
#define VULKAN_BUFFER_H_
#include <vulkan/vulkan.hpp>
#include "vulkan_device.hpp"

class VulkanBuffer {

        public:
                VulkanBuffer(VulkanDevice &device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
                void map();
                void unmap();
                void write(void *data, VkDeviceSize size, VkDeviceSize offset);
                ~VulkanBuffer();

        private:
                VkBuffer buffer;
                VkDeviceMemory memory;
                VulkanDevice &device;
                VkDeviceSize bufferSize;
                void *mapped;
};
#endif // VULKAN_BUFFER_H_
