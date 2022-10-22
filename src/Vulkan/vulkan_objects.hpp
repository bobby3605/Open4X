#ifndef VULKAN_OBJECTS_H_
#define VULKAN_OBJECTS_H_
#include "vulkan_buffer.hpp"
#include "vulkan_descriptors.hpp"
#include "vulkan_device.hpp"
#include "vulkan_object.hpp"
#include "vulkan_renderer.hpp"
#include <map>
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

class VulkanObjects {
  public:
    VulkanObjects(VulkanDevice* device, VulkanDescriptors* descriptorManager);
    void bind(VulkanRenderer* renderer);
    void drawIndirect(VulkanRenderer* renderer);

  private:
    std::shared_ptr<StagedBuffer> vertexBuffer;
    std::shared_ptr<StagedBuffer> indexBuffer;
    std::shared_ptr<StagedBuffer> indirectDrawsBuffer;
    std::shared_ptr<SSBOBuffers> ssboBuffers;
    std::vector<std::shared_ptr<VulkanObject>> objects;
    std::vector<std::shared_ptr<VulkanObject>> animatedObjects;
    std::unordered_map<std::string, std::shared_ptr<GLTF>> gltf_models;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<VkDrawIndexedIndirectCommand> indirectDraws;
    std::vector<VkDescriptorImageInfo> samplerInfos;
    std::vector<VkDescriptorImageInfo> imageInfos;
    std::vector<VkDescriptorImageInfo> normalMapInfos;

    VulkanDevice* device;

    VulkanDescriptors* descriptorManager;
    VkDescriptorSet objectSet;
    VkDescriptorSet materialSet;
};

#endif // VULKAN_OBJECTS_H_
