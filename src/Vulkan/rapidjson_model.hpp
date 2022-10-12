#ifndef RAPIDJSON_MODEL_H_
#define RAPIDJSON_MODEL_H_
#include "../../external/rapidjson/document.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <optional>
#include <vector>

using namespace rapidjson;
class RapidJSON_Model {
  public:
    RapidJSON_Model(std::string filePath);

    Document d;
    class Scene {
      public:
        Scene(Value& sceneJSON);

        std::vector<int> nodes;
    };
    std::vector<Scene> scenes;

    class Node {
      public:
        Node(Value& nodeJSON);

        std::optional<int> mesh;
        glm::vec3 translation = glm::vec3(0.0f);
        glm::quat rotation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
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

    class Accessor {};
    std::vector<Accessor> accessors;
};

#endif // RAPIDJSON_MODEL_H_
