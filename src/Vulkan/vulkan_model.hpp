#ifndef VULKAN_MODEL_H_
#define VULKAN_MODEL_H_

#include "../glTF/GLTF.hpp"
#include "vulkan_descriptors.hpp"
#include <cstdint>
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"
#include "vulkan_buffer.hpp"
#include "vulkan_node.hpp"
#include <glm/glm.hpp>
#include <set>
#include <unordered_map>
#include <vulkan/vulkan.hpp>

class VulkanModel {
  public:
    VulkanModel(std::string filePath, uint32_t fileNum, std::shared_ptr<SSBOBuffers> ssboBuffers);
    ~VulkanModel();
    std::shared_ptr<GLTF> model;
    std::unordered_map<int, std::shared_ptr<VulkanMesh>> meshIDMap;
    std::unordered_map<int, int> materialIDMap;
    AABB aabb;
    uint32_t const totalInstanceCount() { return _totalInstanceCounter; }
    void addInstance(uint32_t firstInstanceID, std::shared_ptr<SSBOBuffers> ssboBuffers);
    void uploadModelMatrix(uint32_t firstInstanceID, glm::mat4 modelMatrix, std::shared_ptr<SSBOBuffers> ssboBuffers);
    void updateAnimations();
    bool hasAnimations() { return animatedNodes.size() != 0; }

  private:
    std::optional<VulkanNode*> findNode(int nodeID);
    std::vector<VulkanNode*> rootNodes;
    std::vector<VulkanNode*> animatedNodes;
    uint32_t _totalInstanceCounter = 0;
};

#endif // VULKAN_MODEL_H_
