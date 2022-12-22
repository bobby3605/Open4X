#ifndef VULKAN_OBJECTS_H_
#define VULKAN_OBJECTS_H_
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
    VulkanObjects(VulkanDevice* device, VulkanDescriptors* descriptorManager);
    void bind(VulkanRenderer* renderer);
    void drawIndirect(VulkanRenderer* renderer);
    std::shared_ptr<VulkanObject> getObjectByName(std::string name);
    const std::vector<VkDrawIndexedIndirectCommand>& draws() const { return indirectDraws; }

  private:
    std::shared_ptr<StagedBuffer> vertexBuffer;
    std::shared_ptr<StagedBuffer> indexBuffer;
    std::shared_ptr<StagedBuffer> indirectDrawsBuffer;
    std::shared_ptr<VulkanBuffer> culledIndirectDrawsBuffer;
    std::shared_ptr<VulkanBuffer> culledInstanceIndicesBuffer;
    std::shared_ptr<VulkanBuffer> visibilityBuffer;
    std::shared_ptr<VulkanBuffer> blockCountsBuffer;
    std::shared_ptr<VulkanBuffer> prefixSumBuffer;
    std::shared_ptr<VulkanBuffer> partialSumBuffer;
    std::shared_ptr<SSBOBuffers> ssboBuffers;
    std::vector<std::shared_ptr<VulkanObject>> objects;
    std::vector<std::future<std::shared_ptr<VulkanObject>>> futureObjects;
    std::vector<std::shared_ptr<VulkanObject>> animatedObjects;
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

    VulkanDevice* device;

    VulkanDescriptors* descriptorManager;
};

#endif // VULKAN_OBJECTS_H_
