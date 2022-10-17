#ifndef VULKAN_BUFFER_H_
#define VULKAN_BUFFER_H_
#include "glm/glm.hpp"
#include "vulkan_device.hpp"
#include <cstdint>
#include <memory>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

// TODO
// Refactor this

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
    friend class UniformBuffer;
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
    UniformBuffer(VulkanDevice* device, VkDeviceSize size);
    ~UniformBuffer();
    void write(void* data);
    VkDescriptorBufferInfo getBufferInfo() { return bufferInfo; }
    void* mapped();

  private:
    VulkanBuffer* uniformBuffer;
    VkDescriptorBufferInfo bufferInfo;

    friend class SSBOBuffers;
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

struct MaterialData {
    glm::vec4 baseColorFactor;
    uint32_t texCoordSelector = 0;
    // FIXME:
    // this isn't being set
    uint32_t samplerIndex;
};

struct IndicesData {
    uint32_t objectIndex;
    uint32_t materialIndex;
    uint32_t texCoordIndex;
    uint32_t verticesCount;
};

class SSBOBuffers {
  public:
    SSBOBuffers(VulkanDevice* device, uint32_t count);
    ~SSBOBuffers();
    VkBuffer const ssboBuffer() { return _ssboBuffer->buffer(); }
    VkBuffer const materialBuffer() { return _materialBuffer->buffer(); }
    VkBuffer const indicesBuffer() { return _indicesBuffer->buffer(); }
    VkBuffer const texCoordsBuffer() { return _texCoordsBuffer->buffer(); }
    VkBuffer const samplersBuffer() { return _samplersBuffer->uniformBuffer->buffer; }
    SSBOData* ssboMapped;
    MaterialData* materialMapped;
    VkSampler* samplersMapped;
    glm::vec2* texCoordsMapped;
    // void* because VulkanImage depends on this header
    std::shared_ptr<void> defaultImage;

    IndicesData* indicesMapped;
    int uniqueObjectID = 0;
    uint32_t texSamplersCount = 0;
    // starts at 1 since the default material is made in the constructor
    int uniqueMaterialID = 1;
    // starts at for the default texcoords
    uint32_t texCoordCount = 1;
    VulkanDevice* device;

  private:
    StorageBuffer* _ssboBuffer;
    StorageBuffer* _materialBuffer;
    StorageBuffer* _indicesBuffer;
    UniformBuffer* _samplersBuffer;
    StorageBuffer* _texCoordsBuffer;
};

#endif // VULKAN_BUFFER_H_
