#ifndef VULKAN_BUFFER_H_
#define VULKAN_BUFFER_H_
#include "glm/glm.hpp"
#include "vulkan_device.hpp"
#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

class VulkanBuffer {

  public:
    VulkanBuffer(VulkanDevice* device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    void map();
    void unmap();
    void write(void* data, VkDeviceSize size, VkDeviceSize offset = 0);
    ~VulkanBuffer();
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    const VkDeviceSize size() const { return bufferSize; }

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
    VkBuffer buffer() { return bufferInfo.buffer; }

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
    alignas(16) glm::vec4 baseColorFactor;
    uint32_t samplerIndex;
    uint32_t imageIndex;
    uint32_t normalMapIndex;
    uint normalScale;
    uint32_t metallicRoughnessMapIndex;
    uint metallicFactor;
    uint roughnessFactor;
    uint32_t aoMapIndex;
    uint occlusionStrength;
};

struct InstanceIndicesData {
    uint32_t objectIndex;
};

struct MaterialIndicesData {
    uint32_t materialIndex;
};

class SSBOBuffers {
  public:
    SSBOBuffers(VulkanDevice* device, uint32_t instanceCount, uint32_t drawsCount);
    ~SSBOBuffers();
    VkBuffer const ssboBuffer() { return _ssboBuffer->buffer(); }
    VkBuffer const materialBuffer() { return _materialBuffer->buffer(); }
    VkBuffer const instanceIndicesBuffer() { return _instanceIndicesBuffer->buffer(); }
    VkBuffer const materialIndicesBuffer() { return _materialIndicesBuffer->buffer(); }
    SSBOData* ssboMapped;
    MaterialData* materialMapped;
    // void* because VulkanImage depends on this header
    std::shared_ptr<void> defaultImage;
    std::shared_ptr<void> defaultSampler;
    std::shared_ptr<void> defaultNormalMap;
    std::shared_ptr<void> defaultMetallicRoughnessMap;
    std::shared_ptr<void> defaultAoMap;

    std::atomic<uint32_t> samplersCount = 1;
    std::map<void*, int> uniqueSamplersMap;
    std::atomic<uint32_t> imagesCount = 1;
    std::map<void*, int> uniqueImagesMap;
    std::atomic<uint32_t> normalMapsCount = 1;
    std::map<void*, int> uniqueNormalMapsMap;
    std::atomic<uint32_t> metallicRoughnessMapsCount = 1;
    std::map<void*, int> uniqueMetallicRoughnessMapsMap;
    std::atomic<uint32_t> aoMapsCount = 1;
    std::map<void*, int> uniqueAoMapsMap;
    InstanceIndicesData* instanceIndicesMapped;
    MaterialIndicesData* materialIndicesMapped;
    std::atomic<uint32_t> uniqueObjectID = 0;
    // starts at 1 since the default material is made in the constructor
    std::atomic<uint32_t> uniqueMaterialID = 1;
    std::atomic<uint32_t> currDrawIndex = 0;
    VulkanDevice* device;

  private:
    StorageBuffer* _ssboBuffer;
    StorageBuffer* _materialBuffer;
    StorageBuffer* _instanceIndicesBuffer;
    StorageBuffer* _materialIndicesBuffer;
};

#endif // VULKAN_BUFFER_H_
