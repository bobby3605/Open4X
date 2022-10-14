#ifndef VULKAN_OBJECTS_H_
#define VULKAN_OBJECTS_H_
#include "rapidjson_model.hpp"
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
    ~VulkanObjects();
    void bind(VulkanRenderer* renderer);
    void drawIndirect(VulkanRenderer* renderer);

  private:
    std::shared_ptr<StagedBuffer> vertexBuffer;
    std::shared_ptr<StagedBuffer> indexBuffer;
    std::shared_ptr<StagedBuffer> indirectDrawsBuffer;
    std::shared_ptr<SSBOBuffers> SSBO;
    std::vector<std::shared_ptr<VulkanObject>> objects;
    std::vector<std::shared_ptr<VulkanObject>> animatedObjects;
    std::unordered_map<std::string, std::shared_ptr<RapidJSON_Model>> gltf_models;
    std::vector<Vertex> vertices;
    std::vector<int> indices;
    std::vector<VkDrawIndexedIndirectCommand> indirectDraws;

    VulkanDevice* device;

    void loadImage(std::string path);
    VkSampler imageSampler;
    VkImageView imageView;
    VkImage image;
    VkDeviceMemory imageMemory;
    uint32_t mipLevels;

    VulkanDescriptors* descriptorManager;
    VkDescriptorSet materialSet;
    VkDescriptorSet objectSet;
};

#endif // VULKAN_OBJECTS_H_
