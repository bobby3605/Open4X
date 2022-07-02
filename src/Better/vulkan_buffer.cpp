#include "vulkan_buffer.hpp"
#include "common.hpp"
#include "vulkan_device.hpp"
#include <cstring>
#include <iostream>

VulkanBuffer::VulkanBuffer(VulkanDevice &device, VkDeviceSize size, VkBufferUsageFlags usage,
                           VkMemoryPropertyFlags properties)
    : device{device}, bufferSize{size} {
  device.createBuffer(size, usage, properties, buffer, memory);
}

void VulkanBuffer::map() {
  checkResult(vkMapMemory(device.device(), memory, 0, bufferSize, 0, &mapped), "memory map failed");
  isMapped = true;
}

void VulkanBuffer::unmap() {
  vkUnmapMemory(device.device(), memory);
  isMapped = false;
}

void VulkanBuffer::write(void *data, VkDeviceSize size, VkDeviceSize offset) {
  memcpy(((char *)mapped) + offset, data, size);
}

VulkanBuffer::~VulkanBuffer() {
  if (isMapped)
    unmap();
  vkDestroyBuffer(device.device(), buffer, nullptr);
  vkFreeMemory(device.device(), memory, nullptr);
}
