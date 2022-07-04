#include "vulkan_model.hpp"
#define TINYOBJLOADER_IMPLEMENTATION
#include "../../external/tiny_obj_loader.h"
#include <unordered_map>

VulkanModel::VulkanModel(VulkanDevice *device, std::string model_path) : device{device} {
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn, err;

  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model_path.c_str())) {
    throw std::runtime_error(warn + err);
  }

  std::unordered_map<Vertex, uint32_t> uniqueVertices{};

  for (const auto &shape : shapes) {
    for (const auto &index : shape.mesh.indices) {
      Vertex vertex{};

      vertex.pos = {attrib.vertices[3 * index.vertex_index + 0], attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]};

      vertex.texCoord = {attrib.texcoords[2 * index.texcoord_index + 0],
                         1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};

      vertex.color = {1.0f, 1.0f, 1.0f};

      if (uniqueVertices.count(vertex) == 0) {
        uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
        vertices.push_back(vertex);
      }

      indices.push_back(uniqueVertices[vertex]);
    }
  }

  vertexBuffer = new StagedBuffer(device, (void *)vertices.data(), sizeof(vertices[0]) * vertices.size(),
                                  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

  indexBuffer = new StagedBuffer(device, (void *)indices.data(), sizeof(indices[0]) * indices.size(),
                                 VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

void VulkanModel::draw(VkCommandBuffer commandBuffer) {

  VkBuffer vertexBuffers[] = {vertexBuffer->getBuffer()};
  VkDeviceSize offsets[] = {0};

  vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

  vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);

  vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
}

VulkanModel::~VulkanModel() {
  delete vertexBuffer;
  delete indexBuffer;
}
