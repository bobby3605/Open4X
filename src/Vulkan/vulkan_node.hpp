#ifndef VULKAN_NODE_H_
#define VULKAN_NODE_H_
#include "../glTF/AccessorLoader.hpp"
#include "../glTF/GLTF.hpp"
#include "vulkan_buffer.hpp"
#include "vulkan_image.hpp"
#include <cstdint>
#include <glm/gtx/hash.hpp>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

struct Vertex {
    glm::vec3 pos;
    glm::vec2 texCoord;
    glm::vec3 normal;
    glm::vec4 tangent;

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
        attributeDescriptions[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
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

class VulkanMesh {
  public:
    VulkanMesh(GLTF* model, uint32_t meshID, std::unordered_map<int, int>* materialIDMap, std::shared_ptr<SSBOBuffers> ssboBuffers);
    std::vector<uint32_t> instanceIDs;
    std::mutex instanceIDsMutex;
    uint32_t const meshID() { return _meshID; };
    AABB aabb;

    class Primitive {
      public:
        Primitive(GLTF* model, int meshID, int primitiveID, std::unordered_map<int, int>* materialIDMap,
                  std::shared_ptr<SSBOBuffers> ssboBuffers);
        void uploadMaterial(std::shared_ptr<SSBOBuffers> ssboBuffers);
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        int materialIndex = 0;
        MaterialData materialData{};
        std::shared_ptr<VulkanImage> image;
        std::shared_ptr<VulkanSampler> sampler;
        std::shared_ptr<VulkanImage> normalMap;
        std::shared_ptr<VulkanImage> metallicRoughnessMap;
        std::shared_ptr<VulkanImage> aoMap;
        float normalScale = 1.0f;
        float metallicFactor = 1.0f;
        float roughnessFactor = 1.0f;
        float occlusionStrength = 1.0f;
        AABB aabb;

      private:
        bool unique = 0;
    };
    std::vector<std::shared_ptr<Primitive>> primitives;

  private:
    uint32_t _meshID;
};

class VulkanNode {
  public:
    VulkanNode(std::shared_ptr<GLTF> model, int nodeID, std::unordered_map<int, std::shared_ptr<VulkanMesh>>* meshIDMap,
               std::unordered_map<int, int>* materialIDMap, uint32_t& totalInstanceCount, std::shared_ptr<SSBOBuffers> ssboBuffers);
    ~VulkanNode();
    void setLocationMatrix(glm::mat4 locationMatrix);
    void uploadModelMatrix(uint32_t& objectID, glm::mat4 parentMatrix, std::shared_ptr<SSBOBuffers> ssboBuffers);
    std::shared_ptr<GLTF> model;
    int nodeID;
    int meshID;
    std::optional<std::pair<std::shared_ptr<GLTF::Animation::Channel>, std::shared_ptr<GLTF::Animation::Sampler>>> animationPair;
    std::vector<VulkanNode*> children;
    void updateAABB(glm::mat4 parentMatrix, AABB& aabb);
    std::shared_ptr<VulkanMesh> mesh = nullptr;
    void addInstance(uint32_t& instanceIDIterator, std::shared_ptr<SSBOBuffers> ssboBuffers);
    void updateAnimation();

  protected:
    const glm::mat4* _baseMatrix = nullptr;
    glm::mat4 animationMatrix{1.0f};

    friend class VulkanObject;
};

#endif // VULKAN_NODE_H_
