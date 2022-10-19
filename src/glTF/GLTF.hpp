#ifndef GLTF_H_
#define GLTF_H_
#include "../../external/rapidjson/document.h"
#include <fstream>
#include <glm/detail/qualifier.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <stdexcept>
#include <vector>

using namespace rapidjson;
class GLTF {
  private:
    Document d;

  public:
    GLTF(std::string filePath, uint32_t fileNum);
    std::string const path() { return _path; }

    class Scene {
      public:
        Scene(Value& sceneJSON);

        std::vector<int> nodes;
    };
    std::vector<Scene> scenes;

    class Node {
      public:
        Node(Value& nodeJSON, GLTF* model);

        std::vector<int> children;
        std::optional<int> mesh;
        glm::mat4 matrix = glm::mat4(1.0f);

      private:
        glm::vec3 translation = glm::vec3(0.0f);
        glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);
    };
    std::vector<Node> nodes;

    class Mesh {
      public:
        Mesh(Value& meshJSON);
        class Primitive {
          public:
            Primitive(Value& primitiveJSON);
            class Attributes {
              public:
                Attributes(Value& attributesJSON);
                std::optional<int> position;
                std::optional<int> normal;
                std::vector<int> texcoords;
            };
            std::optional<int> indices;
            std::optional<int> material;
            std::shared_ptr<Attributes> attributes;
        };
        std::vector<Primitive> primitives;
        int instanceCount = 0;
    };
    std::vector<Mesh> meshes;

    class Buffer {
      public:
        Buffer(Value& bufferJSON, std::string path, std::queue<std::vector<unsigned char>>* binaryBuffers = nullptr);
        std::optional<std::string> uri;
        int byteLength;
        std::vector<unsigned char> data;
    };
    std::vector<Buffer> buffers;
    std::queue<std::vector<unsigned char>> binaryBuffers;

    class BufferView {
      public:
        BufferView(Value& bufferViewJSON);
        int buffer;
        int byteOffset = 0;
        int byteLength;
        std::optional<int> byteStride;
        std::optional<int> target;
    };
    std::vector<BufferView> bufferViews;

    class Accessor {
      public:
        Accessor(Value& accessorJSON);
        class Sparse {
          public:
            Sparse(Value& sparseJSON);
            class Index {
              public:
                Index(Value& indexJSON);
                int bufferView;
                int byteOffset;
                int componentType;
            };
            class SparseValue {
              public:
                SparseValue(Value& valueJSON);
                int bufferView;
                int byteOffset;
            };
            int count;
            std::shared_ptr<Index> indices;
            std::shared_ptr<SparseValue> values;
        };
        std::optional<int> bufferView;
        int byteOffset = 0;
        int componentType;
        bool normalized = false;
        int count;
        std::string type;
        std::vector<float> max;
        std::vector<float> min;
        std::optional<Sparse> sparse;
    };
    std::vector<Accessor> accessors;

    class Animation {
      public:
        Animation(Value& animationJSON);
        class Sampler {
          public:
            Sampler(Value& samplerJSON);
            int inputIndex;
            std::vector<float> inputData;
            std::string interpolation;
            int outputIndex;
            std::vector<glm::mat4> outputData;
        };
        class Channel {
          public:
            Channel(Value& channelJSON);
            int sampler;
            class Target {
              public:
                Target(Value& targetJSON);
                int node;
                std::string path;
            };
            std::shared_ptr<Target> target;
        };
        std::vector<std::shared_ptr<Sampler>> samplers;
        std::vector<std::shared_ptr<Channel>> channels;
    };
    std::vector<Animation> animations;

    class Material {
      public:
        Material(Value& materialJSON);
        class PBRMetallicRoughness {
          public:
            PBRMetallicRoughness(Value& pbrMetallicRoughnessJSON);
            glm::vec4 baseColorFactor{1.0f};
            class TextureInfo {
              public:
                TextureInfo(Value& textureInfoJSON);
                int index;
                int texCoord = 0;
            };
            std::optional<std::shared_ptr<TextureInfo>> baseColorTexture;
            float metallicFactor = 1.0f;
            float roughnessFactor = 1.0f;
            std::optional<std::shared_ptr<TextureInfo>> metallicRoughnessTexture;
        };
        std::shared_ptr<PBRMetallicRoughness> pbrMetallicRoughness;
    };
    std::vector<Material> materials;

    class Image {
      public:
        Image(Value& imageJSON);
        std::optional<std::string> uri;
        std::optional<std::string> mimeType;
        std::optional<int> bufferView;
    };
    std::vector<Image> images;

    class Sampler {
      public:
        Sampler(Value& samplerJSON);
        // default to nearest filtering
        int magFilter = 9728;
        int minFilter = 9728;
        // default to repeat wrapping
        int wrapS = 10497;
        int wrapT = 10497;
    };
    std::vector<Sampler> samplers;

    class Texture {
      public:
        Texture(Value& textureJSON);
        int sampler;
        int source;
    };
    std::vector<Texture> textures;

    // file number, meshid, primitiveid -> gl_BaseInstance
    inline static std::map<std::tuple<int, int, int>, int> primitiveBaseInstanceMap;
    uint32_t const fileNum() { return _fileNum; }

    static uint32_t readuint32(std::ifstream& file) {
        uint32_t buffer;
        file.read((char*)&buffer, sizeof(uint32_t));
        return buffer;
    }
    static std::string getFileExtension(std::string filePath) {
        try {
            return filePath.substr(filePath.find_last_of("."));
        } catch (std::exception& e) {
            throw std::runtime_error("failed to get file extension of: " + filePath);
        }
    }

  private:
    uint32_t _fileNum = 0;
    std::string _path;
};

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

template <typename T> static T getComponent(int componentType, unsigned char* data, int offset) {
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
        return AccessorLoaders<T, uint32_t>::getAccessorValue(data, offset);
        break;
    case 5126:
        return AccessorLoaders<T, float>::getAccessorValue(data, offset);
        break;
    default:
        throw std::runtime_error("Unknown component type: " + std::to_string(componentType));
        break;
    }
}

template <typename T> static size_t typeSwitch(std::string type) {
    if (type.compare("SCALAR") == 0) {
        return sizeof(T);
    } else if (type.compare("VEC2") == 0) {
        return sizeof(glm::vec<2, T>);
    } else if (type.compare("VEC3") == 0) {
        return sizeof(glm::vec<3, T>);
    } else if (type.compare("VEC4") == 0) {
        return sizeof(glm::vec<4, T>);
    } else if (type.compare("MAT2") == 0) {
        return sizeof(glm::mat<2, 2, T>);
    } else if (type.compare("MAT3") == 0) {
        return sizeof(glm::mat<3, 3, T>);
    } else if (type.compare("MAT4") == 0) {
        return sizeof(glm::mat<4, 4, T>);
    } else {
        throw std::runtime_error("Unknown type: " + type);
    }
}

static size_t sizeSwitch(uint32_t componentType, std::string type) {
    switch (componentType) {
    case 5120:
        return typeSwitch<char>(type);
        break;
    case 5121:
        return typeSwitch<unsigned char>(type);
        break;
    case 5122:
        return typeSwitch<short>(type);
        break;
    case 5123:
        return typeSwitch<unsigned short>(type);
        break;
    case 5125:
        return typeSwitch<uint32_t>(type);
        break;
    case 5126:
        return typeSwitch<float>(type);
        break;
    default:
        throw std::runtime_error("Unknown component type: " + std::to_string(componentType));
        break;
    }
}

template <typename T> static T loadAccessor(std::shared_ptr<GLTF> model, GLTF::Accessor* accessor, int count_index) {
    GLTF::BufferView bufferView = model->bufferViews[accessor->bufferView.value()];
    int offset = accessor->byteOffset + bufferView.byteOffset +
                 count_index * (bufferView.byteStride.has_value() ? bufferView.byteStride.value()
                                                                  : sizeSwitch(accessor->componentType, accessor->type));
    return getComponent<T>(accessor->componentType, model->buffers[bufferView.buffer].data.data(), offset);
}

#endif // GLTF_H_
