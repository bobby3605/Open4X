#ifndef VULKAN_NODE_H_
#define VULKAN_NODE_H_
#include "../glTF/AccessorLoader.hpp"
#include "../glTF/GLTF.hpp"
#include "vulkan_image.hpp"
#include "vulkan_model.hpp"
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

class VulkanMesh {
  public:
    VulkanMesh(GLTF* model, int meshID, std::map<int, int>* materialIDMap, std::shared_ptr<SSBOBuffers> ssboBuffers,
               std::vector<VkDrawIndexedIndirectCommand>& indirectDraws);
    class Primitive {
      public:
        Primitive(GLTF* model, int meshID, int primitiveID, std::map<int, int>* materialIDMap, std::shared_ptr<SSBOBuffers> ssboBuffers,
                  std::vector<VkDrawIndexedIndirectCommand>& indirectDraws);
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        int materialIndex = 0;
        int gl_BaseInstance = 0;
        // -1 because it should cause a memory error if it hasn't been set
        uint32_t indirectDrawIndex = -1;
        std::shared_ptr<VulkanImage> image;
        std::shared_ptr<VulkanSampler> sampler;
        std::shared_ptr<VulkanImage> normalMap;
        std::shared_ptr<VulkanImage> metallicRoughnessMap;
        std::shared_ptr<VulkanImage> aoMap;
        float normalScale = 1.0f;
        float metallicFactor = 1.0f;
        float roughnessFactor = 1.0f;
        float occlusionStrength = 1.0f;
    };
    std::vector<std::shared_ptr<Primitive>> primitives;
};

class VulkanNode {
  public:
    VulkanNode(std::shared_ptr<GLTF> model, int nodeID, std::map<int, std::shared_ptr<VulkanMesh>>* meshIDMap,
               std::map<int, int>* materialIDMap, std::shared_ptr<SSBOBuffers> ssboBuffers,
               std::vector<VkDrawIndexedIndirectCommand>& indirectDraws);
    void setLocationMatrix(glm::mat4 locationMatrix);
    std::shared_ptr<GLTF> model;
    int nodeID;
    std::optional<int> meshID;
    std::optional<std::pair<std::shared_ptr<GLTF::Animation::Channel>, std::shared_ptr<GLTF::Animation::Sampler>>> animationPair;
    glm::mat4 const modelMatrix();
    std::vector<std::shared_ptr<VulkanNode>> children;
    void updateChildrenMatrices();
    void updateAnimation();

  protected:
    std::optional<glm::mat4*> _modelMatrix;
    glm::mat4 _baseMatrix{1.0f};
    glm::mat4 animationMatrix{1.0f};
    glm::mat4 _locationMatrix{1.0f};

    friend class VulkanObject;
};

#endif // VULKAN_NODE_H_
