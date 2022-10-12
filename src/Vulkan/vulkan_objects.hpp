#ifndef VULKAN_OBJECTS_H_
#define VULKAN_OBJECTS_H_
#include "rapidjson_model.hpp"
#include "vulkan_buffer.hpp"
#include "vulkan_object.hpp"
#include <memory>
#include <vector>

class VulkanObjects {
  public:
    VulkanObjects();

    std::shared_ptr<VulkanBuffer> vertexBuffer;
    std::shared_ptr<VulkanBuffer> indexBuffer;
    std::shared_ptr<StorageBuffer> SSBO;
    std::vector<VulkanObject> objects;
    std::vector<RapidJSON_Model> gltf_models;
};

#endif // VULKAN_OBJECTS_H_
