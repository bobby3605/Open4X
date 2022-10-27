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
    std::string const fileName() { return _fileName; }

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
                std::optional<int> tangent;
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
        class TextureInfo {
          public:
            TextureInfo(Value& textureInfoJSON);
            int index;
            int texCoord = 0;
            float scale = 1.0f;
        };
        class PBRMetallicRoughness {
          public:
            PBRMetallicRoughness(Value& pbrMetallicRoughnessJSON);
            glm::vec4 baseColorFactor{1.0f};
            std::optional<std::shared_ptr<TextureInfo>> baseColorTexture;
            float metallicFactor = 1.0f;
            float roughnessFactor = 1.0f;
            std::optional<std::shared_ptr<TextureInfo>> metallicRoughnessTexture;
        };
        std::shared_ptr<PBRMetallicRoughness> pbrMetallicRoughness;
        std::optional<std::shared_ptr<TextureInfo>> normalTexture;
        std::optional<std::shared_ptr<TextureInfo>> occlusionTexture;
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
        std::optional<int> sampler;
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

    static int baseInstanceCount;
    static int primitiveCount;

  private:
    uint32_t _fileNum = 0;
    std::string _path;
    std::string _fileName;
};

#endif // GLTF_H_
