#ifndef VULKAN_BUFFER_H_
#define VULKAN_BUFFER_H_
#include "vulkan_device.hpp"
#include <vulkan/vulkan.hpp>

class VulkanBuffer {

public:
  VulkanBuffer(VulkanDevice &device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
  void map();
  void unmap();
  void write(void *data, VkDeviceSize size, VkDeviceSize offset);
  ~VulkanBuffer();
  VkBuffer buffer = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;

private:
  VulkanDevice &device;
  VkDeviceSize bufferSize;
  void *mapped = nullptr;
};
#endif // VULKAN_BUFFER_H_
