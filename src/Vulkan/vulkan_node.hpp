#ifndef VULKAN_NODE_H_
#define VULKAN_NODE_H_
#include "rapidjson_model.hpp"
#include "vulkan_model.hpp"
#include <map>
#include <memory>
#include <optional>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

class VulkanMesh {
  public:
    VulkanMesh(std::shared_ptr<RapidJSON_Model> model, int meshID, int gl_BaseInstance);
    class Primitive {
      public:
        Primitive(std::shared_ptr<RapidJSON_Model> model, int meshID, RapidJSON_Model::Mesh::Primitive primitive, int gl_BaseInstance);
        std::vector<Vertex> vertices;
        std::vector<int> indices;
        VkDrawIndexedIndirectCommand indirectDraw;
    };
    std::vector<std::shared_ptr<Primitive>> primitives;
};

class VulkanNode {
  public:
    VulkanNode(std::shared_ptr<RapidJSON_Model> model, int nodeID, std::shared_ptr<std::map<int, std::shared_ptr<VulkanMesh>>> meshIDMap,
               std::shared_ptr<SSBOBuffers> SSBOBuffers);
    void setModelMatrix(glm::mat4 modelMatrix);
    std::shared_ptr<RapidJSON_Model> model;
    int nodeID;
    std::optional<int> meshID;
    std::optional<std::pair<std::shared_ptr<RapidJSON_Model::Animation::Channel>, std::shared_ptr<RapidJSON_Model::Animation::Sampler>>>
        animationPair;
    std::shared_ptr<std::map<int, std::shared_ptr<VulkanMesh>>> meshIDMap;
    glm::mat4 const modelMatrix();
    std::vector<std::shared_ptr<VulkanNode>> children;
    void updateChildrenMatrices();
    void updateAnimation();

  protected:
    std::optional<glm::mat4*> _modelMatrix;
    glm::mat4 _baseMatrix{1.0f};

    friend class VulkanObject;
};

#endif // VULKAN_NODE_H_
