#ifndef GLTF_TYPES_H_
#define GLTF_TYPES_H_
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include "JSON.hpp"
#include <glm/glm.hpp>
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>

namespace gltf {

template <typename T> std::optional<T> findOptional(JSONnode node, std::string key) {
  std::optional<JSONnode> nodeOptional = node.findOptional(key);
  // TODO
  // Make something better than this mess
  // Checks if the node exists
  // If it exists
  // Then it unwraps the node from nodeOptional
  // Then unwraps the optional value from findOptional
  // Then std::get on the unwrapped node to get the key's value
  // Then wraps it back into optional
  // If it doesn't exist
  // Return a blank optional
  return nodeOptional ? std::optional<T>(std::get<T>(nodeOptional.value().value())) : std::optional<T>();
}

template <typename T> std::optional<T> findOptional(JSONnode* node, std::string key) {
  std::optional<JSONnode> nodeOptional = node->findOptional(key);
  // TODO
  // Make something better than this mess
  // Checks if the node exists
  // If it exists
  // Then it unwraps the node from nodeOptional
  // Then unwraps the optional value from findOptional
  // Then std::get on the unwrapped node to get the key's value
  // Then wraps it back into optional
  // If it doesn't exist
  // Return a blank optional
  return nodeOptional ? std::optional<T>(std::get<T>(nodeOptional.value().value())) : std::optional<T>();
}

template <typename T> T find(JSONnode node, std::string key) {
  return std::get<T>(node.find(key).value());
}

template <typename T> T find(JSONnode* node, std::string key) {
  return std::get<T>(node->find(key).value());
}

class Accessor {
public:
Accessor(JSONnode jsonAccessor);
  class Sparse {
  public:
    Sparse(JSONnode jsonSparse);
    class Index {
    public:
    Index(JSONnode jsonIndex);
      int bufferView;
      std::optional<int> byteOffset;
      int componentType;
    };
    class Value {
    public:
    Value(JSONnode jsonValue);
      int bufferView;
      std::optional<int> byteOffset;
    };
    int count;
    std::vector<Index> indices;
    std::vector<Value> values;
  };
  std::optional<int> bufferView;
  std::optional<int> byteOffset;
  int componentType;
  std::optional<bool> normalized;
  int count;
  std::string type;
  std::vector<double> max;
  std::vector<double> min;
  std::optional<Sparse> sparse;
  std::optional<std::string> name;
};

class Asset {
public:
  Asset(JSONnode jsonAsset);
  std::string copyright;
  std::string generator;
  std::string version;
  std::string minVersion;
};

class Buffer {
public:
  Buffer(JSONnode jsonBuffer, std::vector<unsigned char> byteData);
  std::string uri;
  int byteLength;
  std::optional<std::string> name;
  std::vector<unsigned char> data;
};

class BufferView {
public:
  BufferView(JSONnode jsonBufferView);
  int buffer;
  std::optional<int> byteOffset;
  int byteLength;
  std::optional<int> byteStride;
  std::optional<int> target;
  std::optional<std::string> name;
};
class Node {
public:
  Node(JSONnode jsonNode);
  std::optional<std::string> name;
  std::optional<uint> mesh;
  std::vector<uint> children;
  std::optional<glm::vec3> translation;
  std::optional<glm::quat> rotation;
  std::optional<glm::vec3> scale;
  std::optional<glm::mat4> matrix;
  std::optional<uint> skin;
  std::optional<uint> camera;
  std::vector<float> weights;
};

class Mesh {
public:
  Mesh(JSONnode jsonMesh);
  class Primitive {
      public:
      Primitive(JSONnode jsonPrimitive);
    class Attributes {
        public:
        Attributes(JSONnode::nodeVector jsonAttributes);
      std::optional<int> normal;
      std::optional<int> position;
      std::optional<int> tangent;
      std::vector<int> texcoords;
      std::vector<int> colors;
      std::vector<int> joints;
      std::vector<int> weights;
    };
    class Target {
        public:
        Target(JSONnode jsonTarget);
      int normal;
      int position;
      int tangent;
      std::vector<int> texcoords;
      std::vector<int> colors;
    };
    std::unique_ptr<Attributes> attributes;
    std::optional<int> indices;
    std::optional<int> material;
    std::optional<int> mode;
    std::vector<Target> targets;
  };
  std::vector<Primitive> primitives;
  std::vector<float> weights;
  std::optional<std::string> name;
};

class Scene {
public:
  Scene(JSONnode jsonScene);
  std::vector<int> nodes;
  std::optional<std::string> name;
};


}


#endif // GLTF_TYPES_H_
