// DO NOT USE
// USE GLB FILES FOR NOW
#ifndef GLTF_H_
#define GLTF_H_

#include "JSON.hpp"
#include <glm/glm.hpp>
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <unordered_map>
#include <vector>

// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#properties-reference

namespace gltf {

class Scene {
public:
  std::string name;
  std::vector<uint> nodes;
};

class Node {
public:
  std::string name;
  uint mesh;
  std::vector<uint> children;
  glm::vec3 translation;
  glm::quat rotation;
  glm::vec3 scale;
  glm::mat4 matrix;
  uint skin;
  uint camera;
  std::vector<float> weights;
};

class Mesh {
public:
  class Primitive {
    class Attributes {
      uint normal;
      uint position;
      uint tangent;
      std::vector<uint> texcoords;
      std::vector<uint> colors;
      std::vector<uint> joints;
      std::vector<uint> weights;
    };
    class Target {
      uint normal;
      uint position;
      uint tangent;
      std::vector<uint> texcoords;
      std::vector<uint> colors;
    };
    Attributes attributes;
    uint indices;
    uint material;
    uint mode;
    Target targets;
  };
  std::vector<Primitive> primitives;
  std::vector<float> weights;
};

class Skin {
  uint inverseBindMatrices;
  std::vector<uint> joints;
  uint skeleton;
};

class Animation {
  class Channel {
    uint sampler;
    struct Target {
      uint node;
      std::string path;
    } target;
  };
  class Sampler {
    uint input;
    std::string interpolation;
    uint output;
  };
  std::vector<Channel> channels;
  std::vector<Sampler> samplers;
  std::string name;
};

class Buffer {
public:
  std::string uri;
  uint byteLength;
  std::string name;
};

class BufferView {
public:
  uint buffer;
  uint byteOffset;
  uint byteLength;
  uint byteStride;
  uint target;
  std::string name;
};

class Accessor {
public:
  class Sparse {
  public:
    class Index {
    public:
      uint bufferView;
      uint byteOffset;
      uint componentType;
    };
    class Value {
    public:
      uint bufferView;
      uint byteOffset;
    };
    uint count;
    std::vector<Index> indices;
    std::vector<Value> values;
  };
  uint bufferView;
  uint byteOffset;
  uint componentType;
  bool normalized;
  uint count;
  std::string type;
  std::vector<uint> max;
  std::vector<uint> min;
  Sparse sparse;
  std::string name;
};

class Texture {
  uint sampler;
  uint source;
};

class Image {
  std::string uri;
  uint bufferView;
  std::string mimeType;
};

class Sampler {
  uint magFilter;
  uint minFilter;
  uint wrapS;
  uint wrapT;
};

class Material {

  struct PBRMetallicRoughness {
    float baseColorFactor[4];
    float metallicFactor;
    float roughnessFactor;

    struct BaseColorTexture {
      uint index;
      uint texCoord;
    } baseColorTexture;

    struct MetallicRoughnessTexture {
      uint index;
      uint texCoord;
    } metallicRoughnessTexture;

  } pbrMetallicRoughness;

  struct NormalTexture {
    uint scale;
    uint index;
    uint texCoord;
  } normalTexture;

  std::vector<float> emissiveFactor;
  std::string name;
};

class Camera {
  struct Perspective {
    float aspectRatio;
    float yfov;
    float zfar;
    float znear;
  } perspective;
  struct Orthographic {
    float xmag;
    float ymag;
    float zfar;
    float znear;
  } ortographic;
  std::string type;
  std::string name;
};

class Asset {
  std::string copyright;
  std::string generator;
  std::string version;
  std::string minVersion;
};

class GLTF {
public:
  GLTF(std::string filePath);

  const std::vector<Accessor> accessors() { return _accessors; }
  const std::vector<Animation> animations() { return _animations; }
  const Asset asset() { return _asset; }
  const std::vector<Buffer> buffers() { return _buffers; }
  const std::vector<BufferView> bufferViews() { return _bufferViews; }
  const std::vector<Camera> cameras() { return _cameras; }
  const std::vector<std::string> extensionsUsed() { return _extenstionsUsed; }
  const std::vector<std::string> extensionsRequired() { return _extenstionsRequired; }
  const std::vector<Image> images() { return _images; }
  const std::vector<Material> materials() { return _materials; }
  const std::vector<Mesh> meshes() { return _meshes; }
  const std::vector<Node> nodes() { return _nodes; }
  const std::vector<Sampler> samplers() { return _samplers; }
  const uint scene() { return _scene; }
  const std::vector<Scene> scenes() { return _scenes; }
  const std::vector<Skin> skins() { return _skins; }
  const std::vector<Texture> textures() { return _textures; }

private:
  std::vector<Accessor> _accessors;
  std::vector<Animation> _animations;
  Asset _asset;
  std::vector<Buffer> _buffers;
  std::vector<BufferView> _bufferViews;
  std::vector<Camera> _cameras;
  std::vector<std::string> _extenstionsUsed;
  std::vector<std::string> _extenstionsRequired;
  std::vector<Image> _images;
  std::vector<Material> _materials;
  std::vector<Mesh> _meshes;
  std::vector<Node> _nodes;
  std::vector<Sampler> _samplers;
  uint _scene;
  std::vector<Scene> _scenes;
  std::vector<Skin> _skins;
  std::vector<Texture> _textures;

  void accessorsLoader(JSONnode node);
  void animationsLoader(JSONnode node);
  void assetLoader(JSONnode node);
  void buffersLoader(JSONnode node);
  void bufferViewsLoader(JSONnode node);
  void camerasLoader(JSONnode node);
  void extensionsUsedLoader(JSONnode node);
  void extensionsRequiredLoader(JSONnode node);
  void imagesLoader(JSONnode node);
  void materialsLoader(JSONnode node);
  void meshesLoader(JSONnode node);
  void nodesLoader(JSONnode node);
  void samplersLoader(JSONnode node);
  void sceneLoader(JSONnode node);
  void scenesLoader(JSONnode node);
  void skinsLoader(JSONnode node);
  void texturesLoader(JSONnode node);

  const std::unordered_map<std::string, void (GLTF::*)(JSONnode)> nodeJumpTable = {
      {"accessors", &GLTF::accessorsLoader},
      {"animations", &GLTF::animationsLoader},
      {"asset", &GLTF::assetLoader},
      {"buffers", &GLTF::buffersLoader},
      {"bufferViews", &GLTF::bufferViewsLoader},
      {"cameras", &GLTF::camerasLoader},
      {"extensionsUsed", &GLTF::extensionsUsedLoader},
      {"extensionsRequired", &GLTF::extensionsRequiredLoader},
      {"images", &GLTF::imagesLoader},
      {"materials", &GLTF::materialsLoader},
      {"meshes", &GLTF::meshesLoader},
      {"nodes", &GLTF::nodesLoader},
      {"samplers", &GLTF::samplersLoader},
      {"scene", &GLTF::sceneLoader},
      {"scenes", &GLTF::scenesLoader},
      {"skins", &GLTF::skinsLoader},
      {"textures", &GLTF::texturesLoader},
  };
};

} // namespace gltf
#endif // GLTF_H_
