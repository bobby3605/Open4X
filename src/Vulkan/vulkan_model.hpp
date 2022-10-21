#ifndef VULKAN_MODEL_H_
#define VULKAN_MODEL_H_

#include "../glTF/GLTF.hpp"
#include "vulkan_descriptors.hpp"
#include "vulkan_renderer.hpp"
#include <cstdint>
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"
#include "vulkan_buffer.hpp"
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <set>
#include <unordered_map>
#include <vulkan/vulkan.hpp>

struct Vertex {
    glm::vec3 pos;
    glm::vec2 texCoord;
    glm::vec3 normal;
    glm::vec3 tangent;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions;
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, texCoord);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, normal);

        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[3].offset = offsetof(Vertex, tangent);
        return attributeDescriptions;
    }

    bool operator==(const Vertex& other) const { return pos == other.pos; }
};

namespace std {
template <> struct hash<Vertex> {
    size_t operator()(Vertex const& vertex) const { return (hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec2>()(vertex.texCoord)) << 1); }
};
} // namespace std

/*
class VulkanModel {

  public:
    VulkanModel(VulkanDevice* device, VulkanDescriptors* descriptorManager, GLTF* gltf_model);
    VulkanModel(VulkanDevice* device, VulkanDescriptors* descriptorManager, std::string model_path, std::string texture_path);
    ~VulkanModel();

    void draw(VulkanRenderer* renderer, glm::mat4 _modelMatrix);
    void drawIndirect(VulkanRenderer* renderer);

    GLTF* gltf_model;

  private:
    StagedBuffer* vertexBuffer;
    StagedBuffer* indexBuffer;
    VulkanDevice* device;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<VkDrawIndexedIndirectCommand> indirectDraws;
    StagedBuffer* indirectDrawsBuffer = nullptr;

    VkSampler imageSampler;
    VkImageView imageView;
    VkImage image;
    VkDeviceMemory imageMemory;
    uint32_t mipLevels;

    VulkanDescriptors* descriptorManager;
    VkDescriptorSet materialSet;

    void loadImage(std::string path);

    void loadAccessors();
    void loadAnimations();

};
*/

#endif // VULKAN_MODEL_H_
