#ifndef VULKAN_NODE_H_
#define VULKAN_NODE_H_
#include "rapidjson_model.hpp"
#include "vulkan_model.hpp"
#include <map>
#include <memory>
#include <optional>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

class Mesh {
  public:
    Mesh(RapidJSON_Model* model, int meshID, int gl_BaseInstance);
    class Primitive {
      public:
        Primitive(RapidJSON_Model* model, int meshID, RapidJSON_Model::Mesh::Primitive primitive, int gl_BaseInstance);
        std::vector<Vertex> vertices;
        std::vector<int> indices;
        VkDrawIndexedIndirectCommand indirectDraw;
        // Keeps track of the total number of mesh primitives
        static int firstInstance;
    };
    std::vector<std::shared_ptr<Primitive>> primitives;
};

class VulkanNode {
  public:
    VulkanNode(RapidJSON_Model* model, int nodeID, std::shared_ptr<std::map<int, std::shared_ptr<Mesh>>> meshIDMap);
    RapidJSON_Model* model;
    int nodeID;
    std::optional<int> meshID;
    std::shared_ptr<std::map<int, std::shared_ptr<Mesh>>> meshIDMap;
    glm::mat4 const modelMatrix();

  protected:
    glm::mat4 _modelMatrix{1.0f};

    friend class VulkanObject;
};

#endif // VULKAN_NODE_H_
