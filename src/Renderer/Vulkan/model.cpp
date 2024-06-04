#include "model.hpp"
#include "draw.hpp"
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
#include <vulkan/vulkan_core.h>

Model::Model(std::filesystem::path path, DrawAllocators& draw_allocators) {
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
        _scenes.emplace_back(Scene(this, &scene, draw_allocators));
    }
}

Model::Scene::Scene(Model* model, fastgltf::Scene* scene, DrawAllocators& draw_allocators) : _model(model), _scene(scene) {
    _root_node_indices.reserve(scene->nodeIndices.size());
    for (auto node_index : scene->nodeIndices) {
        _root_node_indices.push_back(node_index);
        _model->_nodes.resize(node_index + 1);
        if (!_model->_nodes[node_index].has_value()) {
            _model->_nodes[node_index] = Node(_model, &_model->_asset->nodes[node_index], glm::mat4(1.0f), draw_allocators);
        }
    }
}

Model::Node::Node(Model* model, fastgltf::Node* node, glm::mat4 const& parent_transform, DrawAllocators& draw_allocators)
    : _model(model), _node(node) {
    if (std::holds_alternative<fastgltf::TRS>(node->transform)) {
        fastgltf::TRS trs = std::get<fastgltf::TRS>(node->transform);
        _transform = glm::translate(glm::mat4(1.0f), glm::make_vec3(trs.translation.value_ptr()));
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
        ++_model->_total_instance_data_count;
        _mesh_index = node->meshIndex.value();
        // Create mesh if it doesn't already exist
        _model->_meshes.try_emplace(_mesh_index.value(), model, &_model->_asset->meshes[_mesh_index.value()], draw_allocators);
    }

    _child_node_indices.reserve(_node->children.size());
    for (auto child_index : _node->children) {
        _child_node_indices.push_back(child_index);
        _model->_nodes.resize(child_index + 1);
        if (!_model->_nodes[child_index].has_value()) {
            _model->_nodes[child_index] = Node(_model, &_model->_asset->nodes[child_index], _transform, draw_allocators);
        }
    }
}

Model::Mesh::Mesh(Model* model, fastgltf::Mesh* mesh, DrawAllocators const& draw_allocators) : _model(model), _mesh(mesh) {
    // Load primitives
    _primitives.reserve(mesh->primitives.size());
    for (auto primitive : mesh->primitives) {
        _primitives.emplace_back(_model, &primitive, draw_allocators);
    }
}

Model::Mesh::Primitive::Primitive(Model* model, fastgltf::Primitive* primitive, DrawAllocators const& draw_allocators)
    : _model(model), _primitive(primitive) {
    std::vector<NewVertex> tmp_vertices;
    std::vector<glm::vec3> positions = get_positions();
    tmp_vertices.resize(positions.size());
    for (std::size_t i = 0; i < tmp_vertices.size(); ++i) {
        tmp_vertices[i].pos = positions[i];
    }

    // Default material
    SubAllocation material_alloc;
    material_alloc.offset = 0;
    material_alloc.size = sizeof(NewMaterialData);
    if (primitive->materialIndex.has_value()) {
        if (!model->_material_allocs.contains(primitive->materialIndex.value())) {
            NewMaterialData material_data{};
            fastgltf::Material& material = model->_asset->materials[primitive->materialIndex.value()];
            material_data.base_color_factor = glm::make_vec4(material.pbrData.baseColorFactor.value_ptr());
            std::optional<fastgltf::TextureInfo>& base_color_texture = material.pbrData.baseColorTexture;
            if (base_color_texture.has_value()) {
                std::vector<glm::vec2> texcoords = get_texcoords(base_color_texture.value().texCoordIndex);
                for (std::size_t i = 0; i < tmp_vertices.size(); ++i) {
                    tmp_vertices[i].tex_coord = texcoords[i];
                }
            }
            // Upload material
            material_alloc = draw_allocators.material_data->alloc();
            model->_material_allocs.insert({primitive->materialIndex.value(), material_alloc});
            draw_allocators.material_data->write(material_alloc, &material_data, sizeof(material_data));
        } else {
            material_alloc = model->_material_allocs.at(primitive->materialIndex.value());
        }
    }
    // FIXME:
    // Handle normal textures and normal coordinates

    if (primitive->indicesAccessor.has_value()) {
        _vertices = std::move(tmp_vertices);
        _indices = load_accessor<uint32_t>(_model->_asset, primitive->indicesAccessor.value());
    } else {
        // Generate indices if none exist
        std::unordered_map<NewVertex, uint32_t> unique_vertices;
        // Preallocate
        unique_vertices.reserve(tmp_vertices.size());
        _indices.reserve(tmp_vertices.size());
        _vertices.reserve(tmp_vertices.size());
        for (NewVertex& vertex : tmp_vertices) {
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
    _draw = new Draw(draw_allocators, _vertices, _indices, material_alloc);
}

std::vector<glm::vec3> Model::Mesh::Primitive::get_positions() {
    return load_accessor<glm::vec3>(_model->_asset, _primitive->findAttribute("POSITION")->second);
}

std::vector<glm::vec2> Model::Mesh::Primitive::get_texcoords(std::size_t tex_coord_selector) {
    return load_accessor<glm::vec2>(_model->_asset, _primitive->findAttribute("TEXCOORD_" + std::to_string(tex_coord_selector))->second);
}

void Model::write_instance_data(glm::mat4 const& object_matrix, std::vector<uint32_t> const& instance_ids) {
    size_t id_index = 0;
    for (auto& root_node_index : _scenes[_default_scene]->_root_node_indices) {
        _nodes[root_node_index.value()]->write_instance_data(this, object_matrix, instance_ids, id_index);
    }
}

void Model::Node::write_instance_data(Model* model, glm::mat4 const& object_matrix, std::vector<uint32_t> const& instance_ids,
                                      size_t& id_index) {
    if (_mesh_index.has_value()) {
        for (const auto& primitive : model->_meshes.at(_mesh_index.value()).primitives()) {
            primitive._draw->write_instance_data(instance_ids[id_index++], {object_matrix * _transform});
        }
    }
    for (auto& child_node_index : _child_node_indices) {
        _model->_nodes[child_node_index.value()]->write_instance_data(model, object_matrix, instance_ids, id_index);
    }
}

std::vector<uint32_t> Model::add_instance() {
    std::vector<uint32_t> instance_ids;
    instance_ids.reserve(_total_instance_data_count);
    // NOTE:
    // This needs to traverse the model in the same order that write_instance_data does
    for (auto& root_node_index : _scenes[_default_scene]->_root_node_indices) {
        _nodes[root_node_index.value()]->add_instance(this, instance_ids);
    }
    return instance_ids;
}

void Model::Node::add_instance(Model* model, std::vector<uint32_t>& instance_ids) {
    if (_mesh_index.has_value()) {
        for (const auto& primitive : model->_meshes.at(_mesh_index.value()).primitives()) {
            instance_ids.emplace_back(primitive._draw->add_instance());
        }
    }
    for (auto& child_node_index : _child_node_indices) {
        _model->_nodes[child_node_index.value()]->add_instance(model, instance_ids);
    }
}
