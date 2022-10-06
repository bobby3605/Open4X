#ifndef VULKAN_MODEL_H_
#define VULKAN_MODEL_H_

#include "../glTF/GLTF.hpp"
#include "vulkan_descriptors.hpp"
#include "vulkan_renderer.hpp"
#include <cstdint>
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "vulkan_buffer.hpp"
#include <any>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
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

    static std::array<VkVertexInputAttributeDescription, 3>
    getAttributeDescriptions() {
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

    bool operator==(const Vertex& other) const {
        return pos == other.pos && color == other.color &&
               texCoord == other.texCoord;
    }
};

namespace std {
template <> struct hash<Vertex> {
    size_t operator()(Vertex const& vertex) const {
        return ((hash<glm::vec3>()(vertex.pos) ^
                 (hash<glm::vec3>()(vertex.color) << 1)) >>
                1) ^
               (hash<glm::vec2>()(vertex.texCoord) << 1);
    }
};
} // namespace std

class VulkanModel {

  public:
    VulkanModel(VulkanDevice* device, VulkanDescriptors* descriptorManager,
                gltf::GLTF* gltf_model);
    VulkanModel(VulkanDevice* device, VulkanDescriptors* descriptorManager,
                std::string model_path, std::string texture_path);
    ~VulkanModel();

    void loadImage(std::string path);
    void draw(VulkanRenderer* renderer);

  private:
    StagedBuffer* vertexBuffer;
    StagedBuffer* indexBuffer;
    VulkanDevice* device;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    VkSampler imageSampler;
    VkImageView imageView;
    VkImage image;
    VkDeviceMemory imageMemory;
    uint32_t mipLevels;

    VulkanDescriptors* descriptorManager;
    VkDescriptorSet materialSet;

    gltf::GLTF* gltf_model;

    void loadAccessors();

    glm::vec2 getVec2(unsigned char* ptr, int offset);
    glm::vec3 getVec3(unsigned char* ptr, int offset);
    glm::vec4 getVec4(unsigned char* ptr, int offset);

    template <typename T> T getBufferData(unsigned char* ptr, int offset) {
        return *reinterpret_cast<T*>(ptr + offset);
    }

    template <typename T> glm::vec2 getVec2(unsigned char* ptr, int offset) {

        float x = getBufferData<T>(ptr, offset);
        float y = getBufferData<T>(ptr, offset + sizeof(T));
        return glm::vec2(x, y);
    }

    template <typename T> glm::vec3 getVec3(unsigned char* ptr, int offset) {

        float x = getBufferData<T>(ptr, offset);
        float y = getBufferData<T>(ptr, offset + sizeof(T));
        float z = getBufferData<T>(ptr, offset + 2 * sizeof(T));
        return glm::vec3(x, y, z);
    }

    template <typename T> glm::vec4 getVec4(unsigned char* ptr, int offset) {

        float x = getBufferData<T>(ptr, offset);
        float y = getBufferData<T>(ptr, offset + sizeof(T));
        float z = getBufferData<T>(ptr, offset + 2 * sizeof(T));
        float w = getBufferData<T>(ptr, offset + 3 * sizeof(T));
        return glm::vec4(x, y, z, w);
    }

    template <typename glm::vec2, typename T>
    glm::vec2 getAccessorValue(unsigned char* ptr, int offset) {
        return getVec2<T>(ptr, offset);
    }

    template <typename glm::vec3, typename T>
    glm::vec3 getAccessorValue(unsigned char* ptr, int offset) {
        return getVec3<T>(ptr, offset);
    }

    template <typename glm::vec4, typename T>
    glm::vec3 getAccessorValue(unsigned char* ptr, int offset) {
        return getVec4<T>(ptr, offset);
    }

    template <typename RT, typename T>
    RT getAccessorValue(unsigned char* ptr, int offset) {
        return getBufferData<T>(ptr, offset);
    }

    template <typename T>
    T loadAccessor(gltf::Accessor* accessor, int count_index) {
        gltf::BufferView* bufferView =
            &gltf_model->bufferViews[accessor->bufferView.value()];
        gltf::Buffer* buffer = &gltf_model->buffers[bufferView->buffer];
        int offset =
            accessor->byteOffset + bufferView->byteOffset +
            count_index * (bufferView->byteLength + bufferView->byteStride);
        switch (accessor->componentType) {
        case 5120:
            return getAccessorValue<T, char>(buffer->data.data(), offset);
            break;
        case 5121:
            return getAccessorValue<T, unsigned char>(buffer->data.data(),
                                                      offset);
            break;
        case 5122:
            return getAccessorValue<T, short>(buffer->data.data(), offset);
            break;
        case 5123:
            return getAccessorValue<T, unsigned short>(buffer->data.data(),
                                                       offset);
            break;
        case 5125:
            return getAccessorValue<T, unsigned int>(buffer->data.data(),
                                                     offset);
            break;
        case 5126:
            return getAccessorValue<T, float>(buffer->data.data(), offset);
            break;
        default:
            throw std::runtime_error("Unknown component type: " +
                                     accessor->componentType);
            break;
        }
    }
};

#endif // VULKAN_MODEL_H_
