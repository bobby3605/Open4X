#ifndef VULKAN_MODEL_H_
#define VULKAN_MODEL_H_

#include "../glTF/GLTF.hpp"
#include "vulkan_descriptors.hpp"
#include "vulkan_renderer.hpp"
#include <cstdint>
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"
#include "vulkan_buffer.hpp"
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <set>
#include <unordered_map>
#include <vulkan/vulkan.hpp>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions;
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

        return attributeDescriptions;
    }

    bool operator==(const Vertex& other) const { return pos == other.pos && color == other.color && texCoord == other.texCoord; }
};

namespace std {
template <> struct hash<Vertex> {
    size_t operator()(Vertex const& vertex) const {
        return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
    }
};
} // namespace std

class VulkanModel {

  public:
    VulkanModel(VulkanDevice* device, VulkanDescriptors* descriptorManager, gltf::GLTF* gltf_model);
    VulkanModel(VulkanDevice* device, VulkanDescriptors* descriptorManager, std::string model_path, std::string texture_path);
    ~VulkanModel();

    void draw(VulkanRenderer* renderer, glm::mat4 _modelMatrix);
    void drawIndirect(VulkanRenderer* renderer);

    gltf::GLTF* gltf_model;

  private:
    StagedBuffer* vertexBuffer;
    StagedBuffer* indexBuffer;
    VulkanDevice* device;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<VkDrawIndexedIndirectCommand> indirectDraws;
    StagedBuffer* indirectDrawsBuffer = nullptr;

    VkSampler imageSampler;
    VkImageView imageView;
    VkImage image;
    VkDeviceMemory imageMemory;
    uint32_t mipLevels;

    VulkanDescriptors* descriptorManager;
    VkDescriptorSet materialSet;

    void loadImage(std::string path);

    void loadAccessors();
    void loadAnimations();

    template <typename T> static T getBufferData(unsigned char* ptr, int offset) { return *reinterpret_cast<T*>(ptr + offset); }

    template <typename T> static glm::vec2 getVec2(unsigned char* ptr, int offset) {

        float x = getBufferData<T>(ptr, offset);
        float y = getBufferData<T>(ptr, offset + sizeof(T));
        return glm::vec2(x, y);
    }

    template <typename T> static glm::vec3 getVec3(unsigned char* ptr, int offset) {

        float x = getBufferData<T>(ptr, offset);
        float y = getBufferData<T>(ptr, offset + sizeof(T));
        float z = getBufferData<T>(ptr, offset + 2 * sizeof(T));
        return glm::vec3(x, y, z);
    }

    template <typename T> static glm::vec4 getVec4(unsigned char* ptr, int offset) {

        float x = getBufferData<T>(ptr, offset);
        float y = getBufferData<T>(ptr, offset + sizeof(T));
        float z = getBufferData<T>(ptr, offset + 2 * sizeof(T));
        float w = getBufferData<T>(ptr, offset + 3 * sizeof(T));
        return glm::vec4(x, y, z, w);
    }

    template <typename RT, typename T> struct AccessorLoaders {
        static RT getAccessorValue(unsigned char* ptr, int offset) { return getBufferData<T>(ptr, offset); }
    };

    template <typename T> struct AccessorLoaders<glm::vec2, T> {
        static glm::vec2 getAccessorValue(unsigned char* ptr, int offset) { return getVec2<T>(ptr, offset); }
    };

    template <typename T> struct AccessorLoaders<glm::vec3, T> {
        static glm::vec3 getAccessorValue(unsigned char* ptr, int offset) { return getVec3<T>(ptr, offset); }
    };

    template <typename T> struct AccessorLoaders<glm::vec4, T> {
        static glm::vec4 getAccessorValue(unsigned char* ptr, int offset) { return getVec4<T>(ptr, offset); }
    };

    template <typename T> struct AccessorLoaders<glm::mat2, T> {
        static glm::mat2 getAccessorValue(unsigned char* ptr, int offset) {
            glm::vec2 x = AccessorLoaders<glm::vec2, T>::getAccessorValue(ptr, offset);
            glm::vec2 y = AccessorLoaders<glm::vec2, T>::getAccessorValue(ptr, offset + 2 * sizeof(T));
            // glm matrix constructors for vectors go by column
            // the matrix data is being read by row
            // so the transpose needs to be returned
            return glm::transpose(glm::mat2(x, y));
        }
    };

    template <typename T> struct AccessorLoaders<glm::mat3, T> {
        static glm::mat3 getAccessorValue(unsigned char* ptr, int offset) {
            glm::vec3 x = AccessorLoaders<glm::vec3, T>::getAccessorValue(ptr, offset);
            glm::vec3 y = AccessorLoaders<glm::vec3, T>::getAccessorValue(ptr, offset + 3 * sizeof(T));
            glm::vec3 z = AccessorLoaders<glm::vec3, T>::getAccessorValue(ptr, offset + 2 * 3 * sizeof(T));
            return glm::transpose(glm::mat3(x, y, z));
        }
    };

    template <typename T> struct AccessorLoaders<glm::mat4, T> {
        static glm::mat4 getAccessorValue(unsigned char* ptr, int offset) {
            glm::vec4 x = AccessorLoaders<glm::vec4, T>::getAccessorValue(ptr, offset);
            glm::vec4 y = AccessorLoaders<glm::vec4, T>::getAccessorValue(ptr, offset + 4 * sizeof(T));
            glm::vec4 z = AccessorLoaders<glm::vec4, T>::getAccessorValue(ptr, offset + 2 * 4 * sizeof(T));
            glm::vec4 w = AccessorLoaders<glm::vec4, T>::getAccessorValue(ptr, offset + 3 * 4 * sizeof(T));
            return glm::transpose(glm::mat4(x, y, z, w));
        }
    };

    template <typename T> T getComponent(int componentType, unsigned char* data, int offset) {
        // TODO
        // Check if accessor->type == <T, type>
        switch (componentType) {
        case 5120:
            return AccessorLoaders<T, char>::getAccessorValue(data, offset);
            break;
        case 5121:
            return AccessorLoaders<T, unsigned char>::getAccessorValue(data, offset);
            break;
        case 5122:
            return AccessorLoaders<T, short>::getAccessorValue(data, offset);
            break;
        case 5123:
            return AccessorLoaders<T, unsigned short>::getAccessorValue(data, offset);
            break;
        case 5125:
            return AccessorLoaders<T, unsigned int>::getAccessorValue(data, offset);
            break;
        case 5126:
            return AccessorLoaders<T, float>::getAccessorValue(data, offset);
            break;
        default:
            throw std::runtime_error("Unknown component type: " + std::to_string(componentType));
            break;
        }
    }

    std::unordered_map<gltf::Accessor*, std::unordered_map<int, int>> sparseIndices;

    template <typename T> T loadAccessor(gltf::Accessor* accessor, int count_index) {
        // Load sparse accessor indices if this accessor has a sparse
        // and if it has not already processed the indices
        if (accessor->sparse.has_value() && (sparseIndices.count(accessor) == 0)) {

            gltf::BufferView* sparseIndicesBufferView = &gltf_model->bufferViews[accessor->sparse->indices->bufferView];
            gltf::Buffer* sparseIndicesBuffer = &gltf_model->buffers[sparseIndicesBufferView->buffer];

            for (int sparseCount = 0; sparseCount < accessor->sparse->count; ++sparseCount) {
                int sparseIndexOffset =
                    accessor->sparse->indices->byteOffset + sparseIndicesBufferView->byteOffset +
                    sparseCount * (sparseIndicesBufferView->byteStride + sparseIndicesBufferView->byteLength / accessor->sparse->count);
                // TODO
                // Might not be unsigned short
                sparseIndices[accessor][getComponent<unsigned short>(accessor->sparse->indices->componentType,
                                                                     sparseIndicesBuffer->data.data(), sparseIndexOffset)] = sparseCount;
            }
        }

        gltf::BufferView* bufferView;
        gltf::Buffer* buffer;
        int offset;
        // If the accessor has a sparse
        // get it and use it for the data
        // There should only be 1 sparseCount per count_index per accessor
        if (accessor->sparse.has_value() && (sparseIndices[accessor].count(count_index) == 1)) {
            bufferView = &gltf_model->bufferViews[accessor->sparse->values->bufferView];
            offset = accessor->sparse->indices->byteOffset + bufferView->byteOffset +
                     sparseIndices[accessor][count_index] * (bufferView->byteStride + sizeof(T));
        } else {

            bufferView = &gltf_model->bufferViews[accessor->bufferView.value()];
            offset = accessor->byteOffset + bufferView->byteOffset + count_index * (bufferView->byteStride + sizeof(T));
            // TODO
            // sizeof(T) might not work for everything
            // bufferView->byteLength / accessor->count fails for multiple
            // accessors in one buffer
        }
        buffer = &gltf_model->buffers[bufferView->buffer];
        return getComponent<T>(accessor->componentType, buffer->data.data(), offset);
    }
};

#endif // VULKAN_MODEL_H_
