#include "model.hpp"
#include "fastgltf/core.hpp"
#include "fastgltf/types.hpp"
#include <stdexcept>
#include <string>
#include <unordered_map>
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
            _model->_nodes[node_index] = Node(model, &_model->_asset->nodes[node_index]);
        }
    }
}

Model::Node::Node(Model* model, fastgltf::Node* node) : _model(model), _node(node) {
    // Check if node has mesh
    if (node->meshIndex.has_value()) {
        std::size_t mesh_index = node->meshIndex.value();
        // Ensure that _meshes is allocated for mesh_index
        _model->_meshes.resize(mesh_index + 1);
        // Check if mesh has been created already
        if (!_model->_meshes[mesh_index].has_value()) {
            // Create mesh and add to vector if it doesn't exist
            _model->_meshes[mesh_index] = Mesh(model, &_model->_asset->meshes[mesh_index]);
        }
    }

    _child_node_indices.reserve(_node->children.size());
    for (auto child_index : _node->children) {
        _child_node_indices.push_back(child_index);
        _model->_nodes.resize(child_index + 1);
        if (!_model->_nodes[child_index].has_value()) {
            _model->_nodes[child_index] = Node(_model, &_model->_asset->nodes[child_index]);
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
