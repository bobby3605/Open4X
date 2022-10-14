#ifndef VULKAN_BUFFER_H_
#define VULKAN_BUFFER_H_
#include "glm/glm.hpp"
#include "vulkan_device.hpp"
#include <vulkan/vulkan.hpp>

class VulkanBuffer {

  public:
    VulkanBuffer(VulkanDevice* device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    void map();
    void unmap();
    void write(void* data, VkDeviceSize size, VkDeviceSize offset = 0);
    ~VulkanBuffer();
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;

  private:
    VulkanDevice* device;
    VkDeviceSize bufferSize;
    bool isMapped = false;
    void* mapped = nullptr;

  protected:
    void* getMapped() { return mapped; }

    friend class StorageBuffer;
};

class StagedBuffer {
  public:
    StagedBuffer(VulkanDevice* device, void* data, VkDeviceSize size, VkMemoryPropertyFlags type);
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

class StorageBuffer {
  public:
    StorageBuffer(VulkanDevice* device, VkDeviceSize size);
    ~StorageBuffer();
    VkBuffer buffer() { return storageBuffer->buffer; }
    void* mapped = nullptr;

  private:
    VulkanBuffer* storageBuffer;
};

struct SSBOData {
    glm::mat4 modelMatrix;
};

class SSBOBuffers {
  public:
    SSBOBuffers(VulkanDevice* device, uint32_t count);
    ~SSBOBuffers();
    VkBuffer const buffer() { return _buffer->buffer(); }
    SSBOData* ssboMapped;
    int uniqueSSBOID = 0;
    int gl_BaseInstance = 0;

  private:
    StorageBuffer* _buffer;
    StorageBuffer* _indexBuffer;
};

#endif // VULKAN_BUFFER_H_
