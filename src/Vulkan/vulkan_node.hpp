#ifndef VULKAN_NODE_H_
#define VULKAN_NODE_H_
#include "../glTF/GLTF.hpp"
#include "vulkan_model.hpp"
#include <map>
#include <memory>
#include <optional>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

class VulkanMesh {
  public:
    VulkanMesh(std::shared_ptr<GLTF> model, int meshID, std::shared_ptr<SSBOBuffers> ssboBuffers);
    class Primitive {
      public:
        Primitive(std::shared_ptr<GLTF> model, int meshID, GLTF::Mesh::Primitive primitive, std::shared_ptr<SSBOBuffers> ssboBuffers);
        std::vector<Vertex> vertices;
        std::vector<int> indices;
        VkDrawIndexedIndirectCommand indirectDraw;
        int materialIndex;
    };
    std::vector<std::shared_ptr<Primitive>> primitives;
    int instanceCount = 0;
    int gl_BaseInstance = 0;
};

class VulkanNode {
  public:
    VulkanNode(std::shared_ptr<GLTF> model, int nodeID, std::shared_ptr<std::map<int, std::shared_ptr<VulkanMesh>>> meshIDMap,
               std::shared_ptr<SSBOBuffers> ssboBuffers);
    void setLocationMatrix(glm::mat4 locationMatrix);
    std::shared_ptr<GLTF> model;
    int nodeID;
    std::optional<int> meshID;
    std::optional<std::pair<std::shared_ptr<GLTF::Animation::Channel>, std::shared_ptr<GLTF::Animation::Sampler>>> animationPair;
    std::shared_ptr<std::map<int, std::shared_ptr<VulkanMesh>>> meshIDMap;
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
