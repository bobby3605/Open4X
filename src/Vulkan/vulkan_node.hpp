#ifndef VULKAN_NODE_H_
#define VULKAN_NODE_H_
#include "../glTF/AccessorLoader.hpp"
#include "../glTF/GLTF.hpp"
#include "vulkan_image.hpp"
#include "vulkan_model.hpp"
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

class VulkanMesh {
  public:
    VulkanMesh(GLTF* model, int meshID, std::unordered_map<int, int>* materialIDMap, std::shared_ptr<SSBOBuffers> ssboBuffers);
    std::vector<uint32_t> objectIDs;
    std::mutex objectIDMutex;
    class Primitive {
      public:
        Primitive(GLTF* model, int meshID, int primitiveID, std::unordered_map<int, int>* materialIDMap,
                  std::shared_ptr<SSBOBuffers> ssboBuffers);
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        int materialIndex = 0;
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

class VulkanModel {
  public:
    VulkanModel(std::string filePath, uint32_t fileNum);
    std::shared_ptr<GLTF> model;
    std::unordered_map<int, std::shared_ptr<VulkanMesh>> meshIDMap;
    std::unordered_map<int, int> materialIDMap;
};

class VulkanNode {
  public:
    VulkanNode(std::shared_ptr<GLTF> model, int nodeID, std::unordered_map<int, std::shared_ptr<VulkanMesh>>* meshIDMap,
               std::unordered_map<int, int>* materialIDMap, std::shared_ptr<SSBOBuffers> ssboBuffers, bool duplicate = false);
    ~VulkanNode();
    void setLocationMatrix(glm::mat4 locationMatrix);
    void setLocationMatrix(glm::vec3 newPosition);
    void uploadModelMatrix(std::shared_ptr<SSBOBuffers> ssboBuffers);
    std::shared_ptr<GLTF> model;
    int nodeID;
    int meshID;
    uint32_t objectID = -1;
    std::optional<std::pair<std::shared_ptr<GLTF::Animation::Channel>, std::shared_ptr<GLTF::Animation::Sampler>>> animationPair;
    glm::mat4 const modelMatrix();
    std::vector<VulkanNode*> children;
    void updateChildrenMatrices();
    void updateAnimation();

  protected:
    glm::mat4* _modelMatrix = nullptr;
    glm::mat4* _baseMatrix;
    glm::mat4* animationMatrix = nullptr;

    friend class VulkanObject;
};

#endif // VULKAN_NODE_H_
