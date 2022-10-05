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
                gltf::GLTF gltf_model);
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

    // Template type is the return type only
    // The type read from the buffer is dependent on componentType
    template <typename T>
    T getComponent(int componentType,
                   std::vector<unsigned char>::iterator& ptr) {
        switch (componentType) {
        case 5120:
            return (T)getComponentType<char>(ptr);
            break;
        case 5121:
            return (T)getComponentType<unsigned char>(ptr);
            break;
        case 5122:
            return (T)getComponentType<short>(ptr);
            break;
        case 5123:
            return (T)getComponentType<unsigned short>(ptr);
            break;
        case 5125:
            return (T)getComponentType<unsigned int>(ptr);
            break;
        case 5126:
            return (T)getComponentType<float>(ptr);
            break;
        default:
            throw std::runtime_error("Unknown component type: " +
                                     componentType);
            break;
        }
    }

    // Can't be used as a function argument, since argument evaluation order
    // is undefined and it causes side effects
    template <typename T>
    T getComponentType(std::vector<unsigned char>::iterator& ptr) {
        // ptr is the iterator over the data buffer
        // *ptr is the first unsigned char in the float
        // &*ptr is a reference to the first unsigned char in the type T
        // ptr can't be used instead of &*ptr because it is an iterator,
        // not a standard reference
        // reinterpret_cast<T*>(&*ptr) interprets the unsigned char
        // reference as a T pointer
        // ptr += sizeof(T) increments the pointer to the next value
        T tmp = *reinterpret_cast<T*>(&*ptr);
        ptr += sizeof(T);
        return tmp;
    }
};

#endif // VULKAN_MODEL_H_
