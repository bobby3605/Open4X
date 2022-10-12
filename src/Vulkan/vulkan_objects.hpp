#ifndef VULKAN_OBJECTS_H_
#define VULKAN_OBJECTS_H_
#include "../../external/rapidjson/document.h"
#include "vulkan_buffer.hpp"
#include "vulkan_object.hpp"
#include <memory>
#include <vector>

class VulkanObjects {

    VulkanObjects();

    std::shared_ptr<VulkanBuffer> vertexBuffer;
    std::shared_ptr<VulkanBuffer> indexBuffer;
    std::shared_ptr<StorageBuffer> SSBO;
    std::vector<VulkanObject> objects;
    std::vector<rapidjson::Document> gltf_models;
};

#endif // VULKAN_OBJECTS_H_
