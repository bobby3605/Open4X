#ifndef VULKAN_OBJECTS_H_
#define VULKAN_OBJECTS_H_
#include "common.hpp"
#include "vulkan_buffer.hpp"
#include "vulkan_descriptors.hpp"
#include "vulkan_device.hpp"
#include "vulkan_object.hpp"
#include "vulkan_renderer.hpp"
#include <future>
#include <map>
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

class VulkanObjects {
  public:
    VulkanObjects(VulkanDevice* device, VulkanDescriptors* descriptorManager, Settings settings);
    ~VulkanObjects();
    void bind(VulkanRenderer* renderer);
    void drawIndirect(VulkanRenderer* renderer);
    VulkanObject* getObjectByName(std::string name);
    const std::vector<VkDrawIndexedIndirectCommand>& draws() const { return indirectDraws; }
    int totalInstanceCount() { return _totalInstanceCount; }
    std::shared_ptr<SSBOBuffers> ssboBuffers;

  private:
    std::shared_ptr<VulkanBuffer> vertexBuffer;
    std::shared_ptr<VulkanBuffer> indexBuffer;
    std::shared_ptr<VulkanBuffer> indirectDrawsBuffer;
    std::shared_ptr<VulkanBuffer> culledIndirectDrawsBuffer;
    std::shared_ptr<VulkanBuffer> culledInstanceIndicesBuffer;
    std::shared_ptr<VulkanBuffer> partialSumsBuffer;
    std::shared_ptr<VulkanBuffer> prefixSumBuffer;
    std::shared_ptr<VulkanBuffer> activeLanesBuffer;
    std::vector<VulkanObject*> objects;
    std::vector<std::future<VulkanObject*>> futureObjects;
    std::vector<VulkanModel*> animatedModels;
    std::vector<VulkanObject*> animatedObjects;
    std::unordered_map<std::string, std::shared_ptr<VulkanModel>> models;
    std::vector<std::future<std::shared_ptr<VulkanModel>>> futureModels;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<VkDrawIndexedIndirectCommand> indirectDraws;
    std::vector<VkDescriptorImageInfo> samplerInfos;
    std::vector<VkDescriptorImageInfo> imageInfos;
    std::vector<VkDescriptorImageInfo> normalMapInfos;
    std::vector<VkDescriptorImageInfo> metallicRoughnessMapInfos;
    std::vector<VkDescriptorImageInfo> aoMapInfos;
    int _totalInstanceCount;

    VulkanDevice* device;

    VulkanDescriptors* descriptorManager;
};

#endif // VULKAN_OBJECTS_H_
