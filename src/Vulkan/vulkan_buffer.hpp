#ifndef VULKAN_BUFFER_H_
#define VULKAN_BUFFER_H_
#include "glm/glm.hpp"
#include "vulkan_device.hpp"
#include <atomic>
#include <cstdint>
#include <glm/gtx/quaternion.hpp>
#include <map>
#include <memory>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

class VulkanBuffer {

  public:
    VulkanBuffer(VulkanDevice* device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    static std::shared_ptr<VulkanBuffer> StagedBuffer(VulkanDevice* device, void* data, VkDeviceSize size, VkBufferUsageFlags usageFlags);
    static std::shared_ptr<VulkanBuffer> UniformBuffer(VulkanDevice* device, VkDeviceSize size);
    static std::shared_ptr<VulkanBuffer> StorageBuffer(VulkanDevice* device, VkDeviceSize size,
                                                       VkMemoryPropertyFlags memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                                                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    void map();
    void unmap();
    void write(void* data, VkDeviceSize size, VkDeviceSize offset = 0);
    void write(void* data);
    ~VulkanBuffer();
    VkDeviceSize size() { return _bufferInfo.range; }
    VkDescriptorBufferInfo bufferInfo() { return _bufferInfo; }
    void* mapped() { return _mapped; }
    VkBuffer& buffer() { return _bufferInfo.buffer; }
    VkDeviceMemory& memory() { return _memory; }
    VkBufferUsageFlags usageFlags() { return _usageFlags; }

  private:
    VulkanDevice* device;
    bool isMapped = false;
    void* _mapped = nullptr;
    VkDescriptorBufferInfo _bufferInfo{};
    VkDeviceMemory _memory = VK_NULL_HANDLE;
    VkBufferUsageFlags _usageFlags;
};

struct UniformBufferObject {
    glm::mat4 projView;
    glm::vec3 camPos;
};

struct SSBOData {
    glm::vec3 translation;
    uint pad;
    glm::quat rotation;
    glm::vec3 scale;
    uint pad2;
};

struct MaterialData {
    alignas(16) glm::vec4 baseColorFactor;
    uint32_t samplerIndex;
    uint32_t imageIndex;
    uint32_t normalMapIndex;
    float normalScale;
    uint32_t metallicRoughnessMapIndex;
    float metallicFactor;
    float roughnessFactor;
    uint32_t aoMapIndex;
    float occlusionStrength;
};

class SSBOBuffers {
  public:
    SSBOBuffers(VulkanDevice* device);
    void createMaterialBuffer(uint32_t drawsCount);
    void createInstanceBuffers(uint32_t instanceCount);
    std::shared_ptr<VulkanBuffer> ssboBuffer() { return _ssboBuffer; }
    std::shared_ptr<VulkanBuffer> materialBuffer() { return _materialBuffer; }
    std::shared_ptr<VulkanBuffer> instanceIndicesBuffer() { return _instanceIndicesBuffer; }
    std::shared_ptr<VulkanBuffer> materialIndicesBuffer() { return _materialIndicesBuffer; }
    std::shared_ptr<VulkanBuffer> culledMaterialIndicesBuffer() { return _culledMaterialIndicesBuffer; }
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
    uint32_t* instanceIndicesMapped;
    uint32_t* materialIndicesMapped;
    std::atomic<uint32_t> uniqueInstanceID = 0;
    // starts at 1 since the default material is made in the constructor
    std::atomic<uint32_t> uniqueMaterialID = 1;
    std::atomic<uint32_t> currDrawIndex = 0;
    VulkanDevice* device;

  private:
    std::shared_ptr<VulkanBuffer> _ssboBuffer;
    std::shared_ptr<VulkanBuffer> _materialBuffer;
    std::shared_ptr<VulkanBuffer> _instanceIndicesBuffer;
    std::shared_ptr<VulkanBuffer> _materialIndicesBuffer;
    std::shared_ptr<VulkanBuffer> _culledMaterialIndicesBuffer;
};

#endif // VULKAN_BUFFER_H_
