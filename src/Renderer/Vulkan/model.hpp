#ifndef MODEL_H_
#define MODEL_H_
#include "fastgltf/types.hpp"
#include <optional>
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

template <typename T> std::vector<T> load_accessor(fastgltf::Asset& asset, unsigned long accessor_index) {
    fastgltf::Accessor* accessor = &asset.accessors[accessor_index];
    std::vector<T> data;
    data.reserve(accessor->count);
    for (T element : fastgltf::iterateAccessor<T>(asset, *accessor)) {
        data.push_back(element);
    }
    return data;
}

template <typename T> inline std::vector<T> load_accessor(fastgltf::Asset* asset, unsigned long accessor_index) {
    return load_accessor<T>(*asset, accessor_index);
}

typedef glm::vec2 texcoord;
struct NewVertex {
    glm::vec3 pos = {0, 0, 0};
    glm::vec3 norm = {0, 0, 1};
    texcoord base = {0, 0};
    texcoord normal = {0, 0};
    texcoord tangent = {0, 0};
    bool operator==(const NewVertex& other) const {
        return pos == other.pos && base == other.base && normal == other.normal && tangent == other.tangent;
    }
};

namespace std {
template <> struct hash<NewVertex> {
    size_t operator()(NewVertex const& vertex) const {
        return (hash<glm::vec3>()(vertex.pos) ^ (hash<texcoord>()(vertex.base)) ^ (hash<texcoord>()(vertex.normal)) ^
                (hash<texcoord>()(vertex.tangent)) << 1);
    }
};
} // namespace std

class Model {
  public:
    Model(std::filesystem::path path);
    ~Model();
    class Scene {
      public:
        Scene(Model* model, fastgltf::Scene* scene);

      protected:
        std::vector<std::optional<std::size_t>> _root_node_indices;

      private:
        Model* _model;
        fastgltf::Scene* _scene;
    };

    class Node {
      public:
        Node(Model* _model, fastgltf::Node* node);

      protected:
        std::vector<std::optional<std::size_t>> _child_node_indices;

      private:
        Model* _model;
        fastgltf::Node* _node;
    };

    class Mesh {
      public:
        Mesh(Model* _model, fastgltf::Mesh* mesh);

        class Primitive {
          public:
            Primitive(Model* _model, fastgltf::Primitive* primitive);
            std::vector<NewVertex> _vertices;
            std::vector<uint32_t> _indices;

          private:
            std::vector<glm::vec3> get_positions();
            std::vector<texcoord> get_texcoords(std::size_t tex_coord_selector);
            Model* _model;
            fastgltf::Primitive* _primitive;
        };

      protected:
        std::vector<Primitive> _primitives;

      private:
        Model* _model;
        fastgltf::Mesh* _mesh;
    };

  protected:
    fastgltf::Asset* _asset;
    std::vector<std::optional<Scene>> _scenes;
    std::vector<std::optional<Node>> _nodes;
    std::vector<std::optional<Mesh>> _meshes;

  private:
    // TODO
    // parser pool to re-use across loads, but not across threads
    fastgltf::Parser _parser;
    fastgltf::MappedGltfFile* _data;
    std::size_t _default_scene;
};

#endif // MODEL_H_
