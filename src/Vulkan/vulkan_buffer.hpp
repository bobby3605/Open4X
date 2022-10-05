#ifndef VULKAN_BUFFER_H_
#define VULKAN_BUFFER_H_
#include "glm/glm.hpp"
#include "vulkan_device.hpp"
#include <vulkan/vulkan.hpp>

class VulkanBuffer {

  public:
    VulkanBuffer(VulkanDevice* device, VkDeviceSize size,
                 VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    void map();
    void unmap();
    void write(void* data, VkDeviceSize size, VkDeviceSize offset = 0);
    ~VulkanBuffer();
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;

  private:
    VulkanDevice* device;
    VkDeviceSize bufferSize;
    void* mapped = nullptr;
    bool isMapped = false;
};

class StagedBuffer {
  public:
    StagedBuffer(VulkanDevice* device, void* data, VkDeviceSize size,
                 VkMemoryPropertyFlags type);
    ~StagedBuffer();
    VkBuffer getBuffer() { return stagedBuffer->buffer; }

  private:
    VulkanBuffer* stagedBuffer;
};

struct UniformBufferObject {
    glm::mat4 view;
    glm::mat4 proj;
};

class UniformBuffer {
  public:
    UniformBuffer(VulkanDevice* device);
    ~UniformBuffer();
    void write(void* data);
    VkDescriptorBufferInfo getBufferInfo() { return bufferInfo; }

  private:
    VulkanBuffer* uniformBuffer;
    VkDescriptorBufferInfo bufferInfo;
};

#endif // VULKAN_BUFFER_H_
