#ifndef VULKAN_OBJECTS_H_
#define VULKAN_OBJECTS_H_
#include "common.hpp"
#include "vulkan_buffer.hpp"
#include "vulkan_descriptors.hpp"
#include "vulkan_device.hpp"
#include "vulkan_object.hpp"
#include "vulkan_renderer.hpp"
#include "vulkan_rendergraph.hpp"
#include <future>
#include <map>
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

class VulkanObjects {
  public:
    VulkanObjects(std::shared_ptr<VulkanDevice> device, VulkanRenderGraph* rg, std::shared_ptr<Settings> settings);
    ~VulkanObjects();
    void updateModels();
    VulkanObject* getObjectByName(std::string name);
    const std::vector<VkDrawIndexedIndirectCommand>& draws() const { return indirectDraws; }
    int totalInstanceCount() { return _totalInstanceCount; }
    std::shared_ptr<SSBOBuffers> ssboBuffers;
    ComputePushConstants computePushConstants{};
    VkQueryPool queryPool;

  private:
    std::shared_ptr<VulkanBuffer> vertexBuffer;
    std::shared_ptr<VulkanBuffer> indexBuffer;
    std::shared_ptr<VulkanBuffer> indirectDrawsBuffer;
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
    uint32_t drawCount;

    std::shared_ptr<VulkanDevice> device;

    VulkanDescriptors* descriptorManager;

    struct SpecData {
        uint32_t local_size_x;
        uint32_t subgroup_size;
    } specData;
};

#endif // VULKAN_OBJECTS_H_
