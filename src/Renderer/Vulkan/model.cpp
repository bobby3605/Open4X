#include "model.hpp"
#include "fastgltf/core.hpp"
#include "fastgltf/math.hpp"
#include "fastgltf/types.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

Model::Model(std::filesystem::path path) {
    fastgltf::Expected<fastgltf::MappedGltfFile> maybe_data = fastgltf::MappedGltfFile::FromPath(path);
    if (maybe_data.error() == fastgltf::Error::None) {
        _data = maybe_data.get_if();
    } else {
        throw std::runtime_error("error " + std::string(fastgltf::getErrorMessage(maybe_data.error())) +
                                 " when opening file: " + path.string());
    }

    fastgltf::Options options = fastgltf::Options::LoadExternalBuffers | fastgltf::Options::LoadExternalImages;
    auto asset = _parser.loadGltf(*_data, path.parent_path(), options);
    if (auto error = asset.error(); error != fastgltf::Error::None) {
        throw std::runtime_error("error " + std::string(fastgltf::getErrorMessage(error)) + " when loading gltf: " + path.string());
    }
    _asset = asset.get_if();
    _default_scene = _asset->defaultScene.has_value() ? _asset->defaultScene.value() : 0;
    _scenes.reserve(_asset->scenes.size());
    for (auto scene : _asset->scenes) {
        _scenes.push_back(Scene(this, &scene));
    }
}

Model::~Model() {
    /*
     * TODO:
     * Confirm that I don't need these.
     * I probably don't because fastgltf is memory mapped io
    delete _data;
    delete _asset;
    */
}

Model::Scene::Scene(Model* model, fastgltf::Scene* scene) : _model(model), _scene(scene) {
    _root_node_indices.reserve(scene->nodeIndices.size());
    for (auto node_index : scene->nodeIndices) {
        _root_node_indices.push_back(node_index);
        _model->_nodes.resize(node_index + 1);
        if (!_model->_nodes[node_index].has_value()) {
            _model->_nodes[node_index] = Node(model, &_model->_asset->nodes[node_index], glm::mat4());
        }
    }
}

Model::Node::Node(Model* model, fastgltf::Node* node, glm::mat4 const& parent_transform) : _model(model), _node(node) {
    if (std::holds_alternative<fastgltf::TRS>(node->transform)) {
        fastgltf::TRS trs = std::get<fastgltf::TRS>(node->transform);
        _transform = glm::translate(glm::mat4(), glm::make_vec3(trs.translation.value_ptr()));
        glm::quat r = glm::make_quat(trs.rotation.value_ptr());
        _transform = _transform * glm::toMat4(r);
        _transform = glm::scale(_transform, glm::make_vec3(trs.scale.value_ptr()));
    } else {
        auto& mat = std::get<fastgltf::math::fmat4x4>(node->transform);
        _transform[0] = glm::make_vec4(mat[0].value_ptr());
        _transform[1] = glm::make_vec4(mat[1].value_ptr());
        _transform[2] = glm::make_vec4(mat[2].value_ptr());
        _transform[3] = glm::make_vec4(mat[3].value_ptr());
    }
    _transform = parent_transform * _transform;
    // Check if node has mesh
    if (node->meshIndex.has_value()) {
        ++_model->_model_matrices_size;
        _mesh_index = node->meshIndex.value();
        // Ensure that _meshes is allocated for mesh_index
        _model->_meshes.resize(_mesh_index.value() + 1);
        // Check if mesh has been created already
        if (!_model->_meshes[_mesh_index.value()].has_value()) {
            // Create mesh and add to vector if it doesn't exist
            _model->_meshes[_mesh_index.value()] = Mesh(model, &_model->_asset->meshes[_mesh_index.value()]);
        }
    }

    _child_node_indices.reserve(_node->children.size());
    for (auto child_index : _node->children) {
        _child_node_indices.push_back(child_index);
        _model->_nodes.resize(child_index + 1);
        if (!_model->_nodes[child_index].has_value()) {
            _model->_nodes[child_index] = Node(_model, &_model->_asset->nodes[child_index], _transform);
        }
    }
}

Model::Mesh::Mesh(Model* model, fastgltf::Mesh* mesh) : _model(model), _mesh(mesh) {
    // Load primitives
    _primitives.reserve(mesh->primitives.size());
    for (auto primitive : mesh->primitives) {
        _primitives.push_back(Primitive(_model, &primitive));
    }
}

Model::Mesh::Primitive::Primitive(Model* model, fastgltf::Primitive* primitive) : _model(model), _primitive(primitive) {
    std::vector<NewVertex> vertices;
    std::vector<glm::vec3> positions = get_positions();
    vertices.resize(positions.size());
    for (std::size_t i = 0; i < vertices.size(); ++i) {
        vertices[i].pos = positions[i];
    }

    if (primitive->materialIndex.has_value()) {
        fastgltf::Material& material = model->_asset->materials[primitive->materialIndex.value()];
        std::optional<fastgltf::TextureInfo>& base_color_texture = material.pbrData.baseColorTexture;
        if (base_color_texture.has_value()) {
            std::vector<texcoord> texcoords = get_texcoords(base_color_texture.value().texCoordIndex);
            for (std::size_t i = 0; i < vertices.size(); ++i) {
                vertices[i].base = texcoords[i];
            }
        }
    }
    // FIXME:
    // Handle normal textures and normal coordinates

    if (primitive->indicesAccessor.has_value()) {
        _vertices = std::move(vertices);
        _indices = load_accessor<uint32_t>(_model->_asset, primitive->indicesAccessor.value());
    } else {
        // Generate indices if none exist
        std::unordered_map<NewVertex, uint32_t> unique_vertices;
        // Preallocate
        unique_vertices.reserve(vertices.size());
        _indices.reserve(vertices.size());
        _vertices.reserve(vertices.size());
        for (NewVertex& vertex : vertices) {
            if (unique_vertices.count(vertex) == 0) {
                unique_vertices[vertex] = static_cast<uint32_t>(_vertices.size());
                _vertices.push_back(vertex);
            }
            _indices.push_back(unique_vertices[vertex]);
        }
        // Shrink to fit
        _indices.shrink_to_fit();
        _vertices.shrink_to_fit();
    }
}

std::vector<glm::vec3> Model::Mesh::Primitive::get_positions() {
    return load_accessor<glm::vec3>(_model->_asset, _primitive->findAttribute("POSITION")->second);
}

std::vector<texcoord> Model::Mesh::Primitive::get_texcoords(std::size_t tex_coord_selector) {
    return load_accessor<texcoord>(_model->_asset, _primitive->findAttribute("TEXCOORD_" + std::to_string(tex_coord_selector))->second);
}

void Model::load_instance_data(glm::mat4 const& object_matrix, std::vector<InstanceData>& instance_data) {
    // reset the size, but not capacity, to 0 for push_back
    instance_data.clear();
    for (auto& root_node_index : _scenes[_default_scene]->_root_node_indices) {
        _nodes[root_node_index.value()]->load_instance_data(object_matrix, instance_data);
    }
}

void Model::Node::load_instance_data(glm::mat4 const& object_matrix, std::vector<InstanceData>& instance_data) {
    if (_mesh_index.has_value()) {
        InstanceData instance{};
        instance.model_matrix = object_matrix * _transform;
        instance_data.push_back(instance);
    }
    for (auto& child_node_index : _child_node_indices) {
        _model->_nodes[child_node_index.value()]->load_instance_data(object_matrix, instance_data);
    }
}
