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
            // A bad cast error will be thrown here if T is incompatible with
            // the component type
            // This would occur if the accessor component type is incompatible
            // with the return type
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

    glm::vec2 getVec2(gltf::Accessor accessor,
                      std::vector<unsigned char>::iterator& ptr);
    glm::vec3 getVec3(gltf::Accessor accessor,
                      std::vector<unsigned char>::iterator& ptr);
    glm::vec4 getVec4(gltf::Accessor accessor,
                      std::vector<unsigned char>::iterator& ptr);

    template <typename T>
    T getAccessorChunk(gltf::Accessor accessor,
                       std::vector<unsigned char>::iterator& ptr) {
        std::any output;
        if (accessor.type.compare("SCALAR") == 0) {
            output = getComponent<T>(accessor.componentType, ptr);
        } else if (accessor.type.compare("VEC2") == 0) {
            output = getVec2(accessor, ptr);
        } else if (accessor.type.compare("VEC3") == 0) {
            output = getVec3(accessor, ptr);
        } else if (accessor.type.compare("VEC4") == 0) {
            output = getVec4(accessor, ptr);
        } else if (accessor.type.compare("MAT2") == 0) {
            glm::vec2 x = getVec2(accessor, ptr);
            glm::vec2 y = getVec2(accessor, ptr);
            // glm matrix constructors for vectors go by column
            // the matrix data is being read by row
            // so the transpose needs to be returned
            output = glm::transpose(glm::mat2(x, y));
        } else if (accessor.type.compare("MAT3") == 0) {
            glm::vec3 x = getVec3(accessor, ptr);
            glm::vec3 y = getVec3(accessor, ptr);
            glm::vec3 z = getVec3(accessor, ptr);
            output = glm::transpose(glm::mat3(x, y, z));
        } else if (accessor.type.compare("MAT4") == 0) {
            glm::vec4 x = getVec4(accessor, ptr);
            glm::vec4 y = getVec4(accessor, ptr);
            glm::vec4 z = getVec4(accessor, ptr);
            glm::vec4 w = getVec4(accessor, ptr);
            output = glm::transpose(glm::mat4(x, y, z, w));
        } else {
            throw std::runtime_error("Unknown accessor type: " + accessor.type);
        }
        // std::any_cast throws bad any_cast if output cannot be casted to T
        // This would occur if T is incompatible with the accessor type
        return std::any_cast<T>(output);
    }
};

#endif // VULKAN_MODEL_H_
