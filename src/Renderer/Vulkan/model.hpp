#ifndef MODEL_H_
#define MODEL_H_
#include "../../utils/math.hpp"
#include "../../utils/utils.hpp"
#include "draw.hpp"
#include "fastgltf/types.hpp"
#include "image.hpp"
#include <cstdint>
#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <glm/gtx/quaternion.hpp>
#include <unordered_map>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

template <typename T> std::vector<T> load_accessor(fastgltf::Asset& asset, unsigned long accessor_index) {
    fastgltf::Accessor* accessor = &asset.accessors[accessor_index];
    std::vector<T> data;
    data.reserve(accessor->count);
    auto iterate_accessor = fastgltf::iterateAccessor<T>(asset, *accessor);
    for (T element : iterate_accessor) {
        data.push_back(element);
    }
    return data;
}

template <typename T> inline std::vector<T> load_accessor(fastgltf::Asset* asset, unsigned long accessor_index) {
    return load_accessor<T>(*asset, accessor_index);
}

class Model {
  public:
    Model(std::filesystem::path path, DrawAllocators& draw_allocators, SubAllocation<FixedAllocator, GPUAllocation>* default_material,
          safe_vector<Draw*>& invalid_draws);
    void write_instance_data(glm::mat4 const& object_matrix, std::vector<InstanceAllocPair> const& instances);
    void add_instance(std::vector<InstanceAllocPair>& instances);
    void preallocate(size_t count);
    NewAABB& aabb() { return _aabb; }
    std::string const& path() { return _path; }
    Image const& get_image(size_t image_index) { return _images[image_index]; }

    class Scene {
      public:
        Scene(Model* model, fastgltf::Scene* scene, DrawAllocators& draw_allocators);

      protected:
        std::vector<fast_optional<size_t>> _root_node_indices;

      private:
        Model* _model;
        fastgltf::Scene* _scene;

        friend class Model;
    };

    class Node {
      public:
        Node(Model* _model, fastgltf::Node* node, glm::mat4 const& parent_transform, DrawAllocators& draw_allocators);
        void write_instance_data(Model* _model, glm::mat4 const& object_matrix, std::vector<InstanceAllocPair> const& instances,
                                 size_t& id_index);

      protected:
        void add_instance(Model* model, std::vector<InstanceAllocPair>& instances, size_t& instance_index);
        void preallocate(Model* model, size_t count);
        fast_optional<size_t> _mesh_index;
        std::vector<fast_optional<size_t>> _child_node_indices;
        alignas(32) glm::mat4 _transform;

      private:
        Model* _model;
        fastgltf::Node* _node;

        friend class Model;
    };

    class Mesh {
      public:
        Mesh(Model* _model, fastgltf::Mesh* mesh, DrawAllocators const& draw_allocators);

        class Primitive {
          public:
            Primitive(Model* _model, fastgltf::Primitive* primitive, DrawAllocators const& draw_allocators);
            Draw* _draw;

          protected:
            // TODO
            // vertices should be generic
            // char* and a byte size
            std::vector<NewVertex> _vertices;
            std::vector<uint32_t> _indices;
            NewAABB _aabb;

          public:
            std::vector<NewVertex> const& vertices() const { return _vertices; }
            std::vector<uint32_t> const& indices() const { return _indices; }

          private:
            std::vector<glm::vec3> get_positions();
            std::vector<glm::vec2> get_texcoords(std::size_t tex_coord_selector);
            Model* _model;
            fastgltf::Primitive* _primitive;

            friend class Model;
        };

      protected:
        std::vector<Primitive> _primitives;
        NewAABB _aabb;

      public:
        std::vector<Primitive> const& primitives() const { return _primitives; }

      private:
        Model* _model;
        fastgltf::Mesh* _mesh;

        friend class Model;
    };

  protected:
    fastgltf::Asset* _asset;
    std::vector<fast_optional<Scene*>> _scenes;
    std::vector<fast_optional<Node*>> _nodes;
    std::vector<fast_optional<Mesh*>> _meshes;
    std::unordered_map<uint32_t, SubAllocation<FixedAllocator, GPUAllocation>*> _material_allocs;
    SubAllocation<FixedAllocator, GPUAllocation>* _default_material;
    safe_vector<Draw*>& _invalid_draws;
    NewAABB _aabb;

  private:
    // TODO
    // parser pool to re-use across loads, but not across threads
    fastgltf::Parser _parser;
    fastgltf::MappedGltfFile* _data;
    std::size_t _default_scene;
    std::size_t _total_instance_data_count = 0;
    std::string _path;
    void load_images();
    std::vector<Image> _images;
    void load_samplers();
    std::vector<Sampler> _samplers;
};

#endif // MODEL_H_
