#include "vulkan_model.hpp"
#include "../../external/stb_image.h"
#include "vulkan_renderer.hpp"
#include <algorithm>
#include <cstdint>
#include <glm/fwd.hpp>
#include <set>
#include <stdexcept>
#include <string>
#include <vulkan/vulkan_core.h>
#define TINYOBJLOADER_IMPLEMENTATION
#include "../../external/tiny_obj_loader.h"
#include "common.hpp"
#include "glm/gtx/string_cast.hpp"
#include "vulkan_buffer.hpp"
#include <unordered_map>

glm::vec2 VulkanModel::getVec2(gltf::Accessor accessor,
                               std::vector<unsigned char>::iterator& ptr,
                               int offset) {

    float x = getComponent<float>(accessor.componentType, ptr, offset);
    float y = getComponent<float>(accessor.componentType, ptr, offset);
    return glm::vec2(x, y);
}
glm::vec3 VulkanModel::getVec3(gltf::Accessor accessor,
                               std::vector<unsigned char>::iterator& ptr,
                               int offset) {

    float x = getComponent<float>(accessor.componentType, ptr, offset);
    float y = getComponent<float>(accessor.componentType, ptr, offset);
    float z = getComponent<float>(accessor.componentType, ptr, offset);
    return glm::vec3(x, y, z);
}
glm::vec4 VulkanModel::getVec4(gltf::Accessor accessor,
                               std::vector<unsigned char>::iterator& ptr,
                               int offset) {

    float x = getComponent<float>(accessor.componentType, ptr, offset);
    float y = getComponent<float>(accessor.componentType, ptr, offset);
    float z = getComponent<float>(accessor.componentType, ptr, offset);
    float w = getComponent<float>(accessor.componentType, ptr, offset);
    return glm::vec4(x, y, z, w);
}

VulkanModel::VulkanModel(VulkanDevice* device,
                         VulkanDescriptors* descriptorManager,
                         gltf::GLTF gltf_model)
    : device{device}, descriptorManager{descriptorManager} {

    for (gltf::Mesh mesh : gltf_model.meshes) {
        int indicesOffset = 0;
        for (gltf::Mesh::Primitive primitive : mesh.primitives) {
            for (gltf::Buffer gltfDataBuffer : gltf_model.buffers) {
                std::vector<unsigned char>::iterator ptr =
                    gltfDataBuffer.data.begin();
                while (ptr < gltfDataBuffer.data.end()) {
                    int ptr_index = ptr - gltfDataBuffer.data.begin();
                    // Detemine which buffer view the pointer has indexed into
                    bool foundBuffer = 0;
                    for (int i = 0; i < (int)gltf_model.bufferViews.size();
                         i++) {
                        // TODO
                        // take byteStride into account
                        if ((ptr_index >=
                             gltf_model.bufferViews[i].byteOffset) &&
                            (ptr_index <
                             (gltf_model.bufferViews[i].byteOffset +
                              gltf_model.bufferViews[i].byteLength))) {
                            foundBuffer = 1;
                            // Get the accessor for the found buffer view
                            for (gltf::Accessor accessor :
                                 gltf_model.accessors) {
                                // If accessor bufferView matches the current
                                // bufferView
                                if (accessor.bufferView == i) {
                                    // If index buffer
                                    if (i == primitive.indices.value()) {
                                        indices.push_back(
                                            getAccessorChunk<int>(
                                                accessor, ptr,
                                                accessor.byteOffset) +
                                            indicesOffset);
                                    }
                                    // If vertex buffer
                                    else if (i == primitive.attributes->position
                                                      .value()) {
                                        Vertex vertex;
                                        vertex.pos =
                                            getAccessorChunk<glm::vec3>(
                                                accessor, ptr,
                                                accessor.byteOffset);
                                        vertex.texCoord = {0, 0};
                                        vertex.color = {1.0f, 1.0f, 1.0f};
                                        vertices.push_back(vertex);
                                    }
                                }
                            }
                        }
                    }
                    // If no valid buffer view, increment ptr
                    if (!foundBuffer) {
                        ++ptr;
                    }
                }
            }

            // If the model did not contain an index buffer, generate one
            if ((indices.size() - indicesOffset) == 0) {
                std::unordered_map<Vertex, uint32_t> uniqueVertices{};
                for (Vertex vertex : vertices) {
                    if (uniqueVertices.count(vertex) == 0) {
                        uniqueVertices[vertex] =
                            static_cast<uint32_t>(vertices.size());
                        vertices.push_back(vertex);
                    }
                    indices.push_back(uniqueVertices[vertex]);
                }
            }
        }
        // Update indices offset
        indicesOffset += indices.size() - indicesOffset;
    }

    vertexBuffer = new StagedBuffer(device, (void*)vertices.data(),
                                    sizeof(vertices[0]) * vertices.size(),
                                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    indexBuffer = new StagedBuffer(device, (void*)indices.data(),
                                   sizeof(indices[0]) * indices.size(),
                                   VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    loadImage("assets/textures/white.png");
}

VulkanModel::VulkanModel(VulkanDevice* device,
                         VulkanDescriptors* descriptorManager,
                         std::string model_path, std::string texture_path)
    : device{device}, descriptorManager{descriptorManager} {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                          model_path.c_str())) {
        throw std::runtime_error(warn + err);
    }

    std::unordered_map<Vertex, uint32_t> uniqueVertices{};

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};

            vertex.pos = {attrib.vertices[3 * index.vertex_index + 0],
                          attrib.vertices[3 * index.vertex_index + 1],
                          attrib.vertices[3 * index.vertex_index + 2]};

            vertex.texCoord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};

            vertex.color = {1.0f, 1.0f, 1.0f};

            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }

            indices.push_back(uniqueVertices[vertex]);
        }
    }

    vertexBuffer = new StagedBuffer(device, (void*)vertices.data(),
                                    sizeof(vertices[0]) * vertices.size(),
                                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    indexBuffer = new StagedBuffer(device, (void*)indices.data(),
                                   sizeof(indices[0]) * indices.size(),
                                   VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    loadImage(texture_path);
}

void VulkanModel::loadImage(std::string path) {

    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight,
                                &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
        throw std::runtime_error("failed to load texture image");
    }

    mipLevels = static_cast<uint32_t>(
                    std::floor(std::log2(std::max(texWidth, texHeight)))) +
                1;

    VulkanBuffer stagingBuffer(device, imageSize,
                               VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    stagingBuffer.map();
    stagingBuffer.write(pixels, imageSize);
    stagingBuffer.unmap();

    stbi_image_free(pixels);

    device->createImage(
        texWidth, texHeight, mipLevels, VK_SAMPLE_COUNT_1_BIT,
        VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, imageMemory);

    device->singleTimeCommands()
        .transitionImageLayout(image, VK_FORMAT_R8G8B8A8_SRGB,
                               VK_IMAGE_LAYOUT_UNDEFINED,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels)
        .copyBufferToImage(stagingBuffer.buffer, image,
                           static_cast<uint32_t>(texWidth),
                           static_cast<uint32_t>(texHeight))
        // TODO
        // Save and load mipmaps from a file
        .generateMipmaps(image, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight,
                         mipLevels)
        .run();

    imageView = device->createImageView(image, VK_FORMAT_R8G8B8A8_SRGB,
                                        VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(device->getPhysicalDevice(), &properties);
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<float>(mipLevels);

    checkResult(
        vkCreateSampler(device->device(), &samplerInfo, nullptr, &imageSampler),
        "failed to create texture sampler!");

    materialSet =
        descriptorManager->allocateSet(descriptorManager->getMaterial());

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = imageView;
    imageInfo.sampler = imageSampler;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = materialSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(device->device(), 1, &descriptorWrite, 0, nullptr);
}

void VulkanModel::draw(VulkanRenderer* renderer) {

    VkBuffer vertexBuffers[] = {vertexBuffer->getBuffer()};
    VkDeviceSize offsets[] = {0};

    renderer->bindDescriptorSet(1, materialSet);

    vkCmdBindVertexBuffers(renderer->getCurrentCommandBuffer(), 0, 1,
                           vertexBuffers, offsets);

    vkCmdBindIndexBuffer(renderer->getCurrentCommandBuffer(),
                         indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(renderer->getCurrentCommandBuffer(),
                     static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
}

VulkanModel::~VulkanModel() {
    delete vertexBuffer;
    delete indexBuffer;

    vkDestroySampler(device->device(), imageSampler, nullptr);
    vkDestroyImageView(device->device(), imageView, nullptr);
    vkDestroyImage(device->device(), image, nullptr);
    vkFreeMemory(device->device(), imageMemory, nullptr);
}
