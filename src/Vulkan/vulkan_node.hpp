#ifndef VULKAN_NODE_H_
#define VULKAN_NODE_H_
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
    VulkanMesh(std::shared_ptr<GLTF> model, int meshID, std::map<int, int>* materialIDMap, std::shared_ptr<SSBOBuffers> ssboBuffers);
    class Primitive {
      public:
        Primitive(std::shared_ptr<GLTF> model, int meshID, int primitiveID, std::map<int, int>* materialIDMap,
                  std::shared_ptr<SSBOBuffers> ssboBuffers);
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        VkDrawIndexedIndirectCommand indirectDraw;
        int materialIndex = 0;
        int gl_BaseInstance = 0;
        std::shared_ptr<VulkanImage> image;
        std::shared_ptr<VulkanSampler> sampler;
    };
    std::vector<std::shared_ptr<Primitive>> primitives;
};

class VulkanNode {
  public:
    VulkanNode(std::shared_ptr<GLTF> model, int nodeID, std::map<int, std::shared_ptr<VulkanMesh>>* meshIDMap,
               std::map<int, int>* materialIDMap, std::shared_ptr<SSBOBuffers> ssboBuffers);
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
