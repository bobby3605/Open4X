#ifndef GLTF_H_
#define GLTF_H_
#include "../../external/rapidjson/document.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <optional>
#include <vector>

using namespace rapidjson;
class GLTF {
  private:
    Document d;

  public:
    GLTF(std::string filePath);

    class Scene {
      public:
        Scene(Value& sceneJSON);

        std::vector<int> nodes;
    };
    std::vector<Scene> scenes;

    class Node {
      public:
        Node(Value& nodeJSON);

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
            };
            std::optional<int> indices;
            std::shared_ptr<Attributes> attributes;
        };
        std::vector<Primitive> primitives;
    };
    std::vector<Mesh> meshes;

    class Buffer {
      public:
        Buffer(Value& bufferJSON);
        std::optional<std::string> uri;
        int byteLength;
        std::vector<unsigned char> data;
    };
    std::vector<Buffer> buffers;

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
};

template <typename T> static T loadAccessor(std::shared_ptr<GLTF> model, GLTF::Accessor* accessor, int count_index) {
    GLTF::BufferView bufferView = model->bufferViews[accessor->bufferView.value()];
    int offset = accessor->byteOffset + bufferView.byteOffset +
                 count_index * (bufferView.byteStride.has_value() ? bufferView.byteStride.value() : sizeof(T));
    return *(reinterpret_cast<T*>(model->buffers[bufferView.buffer].data.data() + offset));
}

#endif // GLTF_H_
