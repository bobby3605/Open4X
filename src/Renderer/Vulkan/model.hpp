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
#include <vulkan/vulkan.hpp>

struct InstanceData {
    glm::mat4 model_matrix;
};

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

    static VkVertexInputBindingDescription binding_description() {
        VkVertexInputBindingDescription binding_description{};
        binding_description.binding = 0;
        binding_description.stride = sizeof(NewVertex);
        binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return binding_description;
    }

    static std::array<VkVertexInputAttributeDescription, 4> attribute_descriptions() {
        std::array<VkVertexInputAttributeDescription, 4> attribute_descriptions;
        attribute_descriptions[0].binding = 0;
        attribute_descriptions[0].location = 0;
        attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attribute_descriptions[0].offset = offsetof(NewVertex, pos);

        attribute_descriptions[1].binding = 0;
        attribute_descriptions[1].location = 1;
        attribute_descriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
        attribute_descriptions[1].offset = offsetof(NewVertex, base);

        attribute_descriptions[2].binding = 0;
        attribute_descriptions[2].location = 2;
        attribute_descriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        attribute_descriptions[2].offset = offsetof(NewVertex, normal);

        attribute_descriptions[3].binding = 0;
        attribute_descriptions[3].location = 3;
        attribute_descriptions[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attribute_descriptions[3].offset = offsetof(NewVertex, tangent);
        return attribute_descriptions;
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

    void load_instance_data(glm::mat4 const& object_matrix, std::vector<InstanceData>& instance_data);
    const std::size_t model_matrices_size() { return _model_matrices_size; }

    class Scene {
      public:
        Scene(Model* model, fastgltf::Scene* scene);

      protected:
        std::vector<std::optional<std::size_t>> _root_node_indices;

      private:
        Model* _model;
        fastgltf::Scene* _scene;

        friend class Model;
    };

    class Node {
      public:
        Node(Model* _model, fastgltf::Node* node, glm::mat4 const& parent_transform);
        void load_instance_data(glm::mat4 const& object_matrix, std::vector<InstanceData>& instance_data);

      protected:
        std::optional<std::size_t> _mesh_index;
        std::vector<std::optional<std::size_t>> _child_node_indices;
        glm::mat4 _transform;

      private:
        Model* _model;
        fastgltf::Node* _node;

        friend class Model;
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

            friend class Model;
        };

      protected:
        std::vector<Primitive> _primitives;

      private:
        Model* _model;
        fastgltf::Mesh* _mesh;

        friend class Model;
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
    std::size_t _model_matrices_size = 0;
};

#endif // MODEL_H_
