#include "model.hpp"
#include "../../utils/math.hpp"
#include "common.hpp"
#include "draw.hpp"
#include "fastgltf/core.hpp"
#include "fastgltf/math.hpp"
#include "fastgltf/types.hpp"
#include "memory_manager.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include <libintl.h>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <vulkan/vulkan_core.h>

Model::Model(std::filesystem::path path, DrawAllocators& draw_allocators, SubAllocation<FixedAllocator, GPUAllocation>* default_material,
             safe_vector<Draw*>& invalid_draws)
    : _default_material(default_material), _invalid_draws(invalid_draws), _path(path), _draw_allocators(&draw_allocators) {
    fastgltf::Expected<fastgltf::MappedGltfFile> maybe_data = fastgltf::MappedGltfFile::FromPath(path);
    if (maybe_data.error() == fastgltf::Error::None) {
        _data = maybe_data.get_if();
    } else {
        throw std::runtime_error("error " + std::string(fastgltf::getErrorMessage(maybe_data.error())) +
                                 " when opening file: " + path.string());
    }

    fastgltf::Options options = fastgltf::Options::LoadExternalBuffers | fastgltf::Options::LoadExternalImages;
    // NOTE:
    // asset and _asset are deallocated once the constructor returns
    auto asset = _parser.loadGltf(*_data, path.parent_path(), options);
    if (auto error = asset.error(); error != fastgltf::Error::None) {
        throw std::runtime_error("error " + std::string(fastgltf::getErrorMessage(error)) + " when loading gltf: " + path.string());
    }
    _asset = asset.get_if();

    // Pre load all images
    // This is less complex than loading images on demand,
    // because the Model doesn't need to keep track of which images exist
    // It allows allows for further optimization by creating images in bulk
    // If needed, gltf files can be optimized to get rid of any unused images
    load_textures();
    load_samplers();
    load_materials(draw_allocators);
    load_meshes(draw_allocators);

    _default_scene = _asset->defaultScene.has_value() ? *_asset->defaultScene : 0;
    _scenes.resize(_asset->scenes.size());
    _nodes.resize(_asset->nodes.size());
    for (size_t i = 0; i < _asset->scenes.size(); ++i) {
        _scenes[i] = new Scene(this, &_asset->scenes[i], draw_allocators);
    }
    if (_asset->animations.size() > 0) {
        _has_animations = true;
    }
    // NOTE:
    // This must happen after Scene is created,
    // because it needs the nodes to exist
    load_animations();
}

Model::~Model() {
    for (auto scene : _scenes) {
        if (scene.has_value()) {
            delete scene.value();
        }
    }
    for (auto mesh : _meshes) {
        if (mesh.has_value()) {
            delete mesh.value();
        }
    }
    for (auto node : _nodes) {
        if (node.has_value()) {
            delete node.value();
        }
    }
    for (auto texture : _textures) {
        delete texture;
    }
    for (auto sampler : _samplers) {
        delete sampler;
    }
    for (auto material_alloc : _material_allocs) {
        NewMaterialData material_data{};
        material_alloc->get(&material_data);
        std::vector<VkDescriptorImageInfo>& textures = MemoryManager::memory_manager->global_image_infos["textures"];
        std::vector<VkDescriptorImageInfo>& samplers = MemoryManager::memory_manager->global_image_infos["samplers"];
        if (material_data.base_texture_index != 0) {
            textures.erase(textures.begin() + material_data.base_texture_index);
        }
        if (material_data.normal_index != 0) {
            textures.erase(textures.begin() + material_data.normal_index);
        }
        if (material_data.metallic_roughness_index != 0) {
            textures.erase(textures.begin() + material_data.metallic_roughness_index);
        }
        if (material_data.ao_index != 0) {
            textures.erase(textures.begin() + material_data.ao_index);
        }
        if (material_data.sampler_index != 0) {
            samplers.erase(samplers.begin() + material_data.sampler_index);
        }
        _draw_allocators->material_data->free(material_alloc);
    }
    // TODO:
    // cleanup fastgltf asset
    //  delete _asset;
}

void Model::load_textures() {
    for (size_t i = 0; i < _asset->images.size(); ++i) {
        _textures.push_back(new Texture(*_asset, i, _path));
    }
}

void Model::load_samplers() {
    for (size_t i = 0; i < _asset->samplers.size(); ++i) {
        _samplers.push_back(new Sampler(*_asset, i));
    }
}

size_t Model::upload_texture(size_t texture_index) {
    fastgltf::Texture const& gltf_texture = _asset->textures[texture_index];
    if (gltf_texture.imageIndex.has_value()) {
        Texture const* texture = get_texture(gltf_texture.imageIndex.value());
        MemoryManager::memory_manager->global_image_infos["textures"].push_back(texture->image_info());
        // TODO
        // not thread safe
        return MemoryManager::memory_manager->global_image_infos["textures"].size() - 1;
    } else {
        throw std::runtime_error("unsupported texture without image index" + path() + " texture index:  " + std::to_string(texture_index));
    }
};

void Model::load_materials(DrawAllocators& draw_allocators) {
    _material_allocs.resize(_asset->materials.size());
    for (size_t material_index = 0; material_index < _asset->materials.size(); ++material_index) {
        // NOTE:
        // texture indices all default to 0
        NewMaterialData material_data{};
        fastgltf::Material const& material = _asset->materials[material_index];
        fastgltf::PBRData const& pbrData = material.pbrData;
        material_data.base_color_factor = glm::make_vec4(pbrData.baseColorFactor.value_ptr());

        // Upload sampler and check if the material uses multiple samplers
        std::optional<size_t> gltf_sampler_index;
        auto upload_sampler = [&](size_t texture_index) {
            fastgltf::Optional<size_t> const& sampler_index = _asset->textures[texture_index].samplerIndex;
            if (sampler_index.has_value()) {
                if (!gltf_sampler_index.has_value()) {
                    MemoryManager::memory_manager->global_image_infos["samplers"].push_back(_samplers[sampler_index.value()]->image_info());
                    material_data.sampler_index = MemoryManager::memory_manager->global_image_infos["samplers"].size() - 1;
                    gltf_sampler_index = sampler_index.value();
                } else if (gltf_sampler_index.value() != sampler_index.value()) {
                    std::cout << "warning: "
                              << "model: " << path() << "material: " << material_index
                              << " contains multiple samplers, which is unsupported" << std::endl;
                }
            }
        };

        // Load base texture
        if (material.pbrData.baseColorTexture.has_value()) {
            upload_sampler(pbrData.baseColorTexture.value().textureIndex);
            // load texture
            material_data.base_texture_index = upload_texture(pbrData.baseColorTexture.value().textureIndex);
        }
        // Load normal map
        if (material.normalTexture.has_value()) {
            material_data.normal_scale = material.normalTexture.value().scale;
            material_data.normal_index = upload_texture(material.normalTexture.value().textureIndex);
            upload_sampler(material.normalTexture.value().textureIndex);
        }
        // Load metallic roughness
        material_data.metallic_factor = material.pbrData.metallicFactor;
        material_data.roughness_factor = material.pbrData.roughnessFactor;
        if (material.pbrData.metallicRoughnessTexture.has_value()) {
            material_data.metallic_roughness_index = upload_texture(material.pbrData.metallicRoughnessTexture.value().textureIndex);
            upload_sampler(material.pbrData.metallicRoughnessTexture.value().textureIndex);
        }
        // Load ambient occlusion
        if (material.occlusionTexture.has_value()) {
            material_data.occlusion_strength = material.occlusionTexture.value().strength;
            material_data.ao_index = upload_texture(material.occlusionTexture.value().textureIndex);
            upload_sampler(material.occlusionTexture.value().textureIndex);
        }
        // Upload material
        _material_allocs[material_index] = draw_allocators.material_data->alloc();
        _material_allocs[material_index]->write(&material_data);
    }
}

void Model::load_meshes(DrawAllocators& draw_allocators) {
    _meshes.resize(_asset->meshes.size());
    for (size_t i = 0; i < _asset->meshes.size(); ++i) {
        _meshes[i] = new Mesh(this, &_asset->meshes[i], draw_allocators);
    }
}

Model::Scene::Scene(Model* model, fastgltf::Scene* scene, DrawAllocators& draw_allocators) : _model(model), _scene(scene) {
    _root_node_indices.reserve(scene->nodeIndices.size());
    for (auto node_index : scene->nodeIndices) {
        _root_node_indices.push_back(node_index);
        if (!_model->_nodes[node_index].has_value()) {
            _model->_nodes[node_index] = new Node(_model, &_model->_asset->nodes[node_index], glm::mat4(1.0f), draw_allocators);
        }
    }
}

Model::Node::Node(Model* model, fastgltf::Node* node, glm::mat4 const& parent_transform, DrawAllocators& draw_allocators)
    : _model(model), _node(node) {
    if (std::holds_alternative<fastgltf::TRS>(node->transform)) {
        fastgltf::TRS trs = std::get<fastgltf::TRS>(node->transform);
        fast_trs_matrix(glm::make_vec3(trs.translation.value_ptr()), glm::make_quat(trs.rotation.value_ptr()),
                        glm::make_vec3(trs.scale.value_ptr()), _transform);
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
        _mesh_index = node->meshIndex.value();
        std::optional<Mesh*> mesh_opt = _model->_meshes[_mesh_index.value()];
        if (mesh_opt.has_value()) {
            Mesh* mesh = mesh_opt.value();
            _model->_total_instance_data_count += mesh->primitives().size();
            _model->_aabb.update(_transform * glm::vec4(mesh->_aabb.max(), 1.0f));
            _model->_aabb.update(_transform * glm::vec4(mesh->_aabb.min(), 1.0f));
        } else {
            throw std::runtime_error("error: malformed gltf: mesh id: " + std::to_string(_mesh_index.value()) +
                                     " doesn't exist: " + model->path());
        }
    }

    _child_node_indices.reserve(_node->children.size());
    for (auto child_index : _node->children) {
        _child_node_indices.push_back(child_index);
        if (!_model->_nodes[child_index].has_value()) {
            _model->_nodes[child_index] = new Node(_model, &_model->_asset->nodes[child_index], _transform, draw_allocators);
        }
    }
}

Model::Mesh::Mesh(Model* model, fastgltf::Mesh* mesh, DrawAllocators const& draw_allocators) : _model(model), _mesh(mesh) {
    // Load primitives
    _primitives.reserve(mesh->primitives.size());
    _model->_invalid_draws.grow(_primitives.capacity());
    for (auto primitive : mesh->primitives) {
        _primitives.emplace_back(_model, &primitive, draw_allocators);
        _aabb.update(_primitives.back()._aabb);
    }
}

Model::Mesh::Primitive::Primitive(Model* model, fastgltf::Primitive* primitive, DrawAllocators const& draw_allocators)
    : _model(model), _primitive(primitive) {
    std::vector<NewVertex> tmp_vertices;
    std::vector<glm::vec3> positions = get_positions();
    std::vector<glm::vec3> normals = get_normals();
    tmp_vertices.resize(positions.size());

    // Get material and texture coordinates
    std::vector<glm::vec2> texcoords(0);
    SubAllocation<FixedAllocator, GPUAllocation>* material_alloc = model->_default_material;
    if (primitive->materialIndex.has_value()) {
        material_alloc = model->_material_allocs[primitive->materialIndex.value()];
        fastgltf::Material const& material = model->_asset->materials[primitive->materialIndex.value()];
        if (material.pbrData.baseColorTexture.has_value()) {
            // Load texcoords into vertices
            texcoords = get_texcoords(material.pbrData.baseColorTexture.value().texCoordIndex);
            for (std::size_t i = 0; i < tmp_vertices.size(); ++i) {
                tmp_vertices[i].tex_coord = texcoords[i];
            }
        }
    }

    // load vertices
    for (std::size_t i = 0; i < tmp_vertices.size(); ++i) {
        tmp_vertices[i].pos = positions[i];
        tmp_vertices[i].normal = normals[i];
        if (texcoords.size() > 0) {
            tmp_vertices[i].tex_coord = texcoords[i];
        }
        _aabb.update(tmp_vertices[i].pos);
    }

    if (primitive->indicesAccessor.has_value()) {
        _vertices = std::move(tmp_vertices);
        _indices = load_accessor<uint32_t>(_model->_asset, *primitive->indicesAccessor);
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

    _draw = new Draw(draw_allocators, _vertices, _indices, material_alloc, model->_invalid_draws);
}

Model::Mesh::Primitive::~Primitive() { delete _draw; }

std::vector<glm::vec3> Model::Mesh::Primitive::get_positions() {
    return load_accessor<glm::vec3>(_model->_asset, _primitive->findAttribute("POSITION")->second);
}

std::vector<glm::vec3> Model::Mesh::Primitive::get_normals() {
    return load_accessor<glm::vec3>(_model->_asset, _primitive->findAttribute("NORMAL")->second);
}

std::vector<glm::vec2> Model::Mesh::Primitive::get_texcoords(std::size_t tex_coord_selector) {
    return load_accessor<glm::vec2>(_model->_asset, _primitive->findAttribute("TEXCOORD_" + std::to_string(tex_coord_selector))->second);
}

void Model::write_instance_data(glm::mat4 const& object_matrix, std::vector<InstanceAllocPair> const& instances) {
    size_t id_index = 0;
    if (_scenes[_default_scene].has_value()) {
        std::vector<size_t>& root_node_indices = _scenes[_default_scene].value()->_root_node_indices;
        for (uint32_t i = 0; i < root_node_indices.size(); ++i) {
            if (_nodes[root_node_indices[i]].has_value()) {
                _nodes[root_node_indices[i]].value()->write_instance_data(this, object_matrix, instances, id_index);
            } else {
                throw std::runtime_error("error: malformed gltf: node id: " + std::to_string(root_node_indices[i]) +
                                         " doesn't exist: " + path());
            }
        }
    } else {
        throw std::runtime_error("error: malformed gltf: scene id: " + std::to_string(_default_scene) + " doesn't exist: " + path());
    }
}

void Model::Node::write_instance_data(Model* model, glm::mat4 const& object_matrix, std::vector<InstanceAllocPair> const& instances,
                                      size_t& id_index) {
    InstanceData instance_data{};
    if (_mesh_index.has_value()) {
        if (model->_meshes[_mesh_index.value()].has_value()) {
            for (uint32_t i = 0; i < model->_meshes[_mesh_index.value()].value()->_primitives.size(); ++i) {
                fast_mat4_mul(object_matrix, _transform, instance_data.model_matrix);
                instances[id_index++].data->write(&instance_data);
            }
        } else {
            throw std::runtime_error("error: malformed gltf: mesh id: " + std::to_string(_mesh_index.value()) +
                                     " doesn't exist: " + _model->path());
        }
    }
    for (uint32_t i = 0; i < _child_node_indices.size(); ++i) {
        if (_model->_nodes[_child_node_indices[i]].has_value()) {
            _model->_nodes[_child_node_indices[i]].value()->write_instance_data(model, object_matrix, instances, id_index);
        } else {
            throw std::runtime_error("error: malformed gltf: node id: " + std::to_string(_child_node_indices[i]) +
                                     " doesn't exist: " + _model->path());
        }
    }
}

void Model::add_instance(std::vector<InstanceAllocPair>& instances) {
    // TODO
    // pre allocate instances,
    // lots of small allocations are slow
    instances.reserve(_total_instance_data_count);
    size_t instance_index = 0;
    // NOTE:
    // This needs to traverse the model in the same order that write_instance_data does
    for (auto& root_node_index : _scenes[_default_scene].value()->_root_node_indices) {
        if (_nodes[root_node_index].has_value()) {
            _nodes[root_node_index].value()->add_instance(this, instances, instance_index);
        } else {
            throw std::runtime_error("error: malformed gltf: node id: " + std::to_string(root_node_index) + " doesn't exist: " + path());
        }
    }
}

void Model::Node::add_instance(Model* model, std::vector<InstanceAllocPair>& instances, size_t& instance_index) {
    if (_mesh_index.has_value()) {
        if (model->_meshes[_mesh_index.value()].has_value()) {
            std::vector<Model::Mesh::Primitive> const& primitives = model->_meshes[_mesh_index.value()].value()->_primitives;
            for (uint32_t i = 0; i < primitives.size(); ++i) {
                primitives[i]._draw->add_instance(instances[instance_index++]);
            }
        } else {
            throw std::runtime_error("error: malformed gltf: mesh id: " + std::to_string(_mesh_index.value()) +
                                     " doesn't exist: " + _model->path());
        }
    }
    for (uint32_t i = 0; i < _child_node_indices.size(); ++i) {
        if (_model->_nodes[_child_node_indices[i]].has_value()) {
            _model->_nodes[_child_node_indices[i]].value()->add_instance(model, instances, instance_index);
        } else {
            throw std::runtime_error("error: malformed gltf: node id: " + std::to_string(_child_node_indices[i]) +
                                     " doesn't exist: " + _model->path());
        }
    }
}

void Model::remove_instance(std::vector<InstanceAllocPair>& instances) {
    size_t instance_index = 0;
    // NOTE:
    // This needs to traverse the model in the same order that add_instance does
    for (auto& root_node_index : _scenes[_default_scene].value()->_root_node_indices) {
        if (_nodes[root_node_index].has_value()) {
            _nodes[root_node_index].value()->remove_instance(this, instances, instance_index);
        } else {
            throw std::runtime_error("error: malformed gltf: node id: " + std::to_string(root_node_index) + " doesn't exist: " + path());
        }
    }
}

void Model::Node::remove_instance(Model* model, std::vector<InstanceAllocPair>& instances, size_t& instance_index) {
    if (_mesh_index.has_value()) {
        if (model->_meshes[_mesh_index.value()].has_value()) {
            std::vector<Model::Mesh::Primitive> const& primitives = model->_meshes[_mesh_index.value()].value()->_primitives;
            for (uint32_t i = 0; i < primitives.size(); ++i) {
                primitives[i]._draw->remove_instance(instances[instance_index++]);
            }
        } else {
            throw std::runtime_error("error: malformed gltf: mesh id: " + std::to_string(_mesh_index.value()) +
                                     " doesn't exist: " + _model->path());
        }
    }
    for (uint32_t i = 0; i < _child_node_indices.size(); ++i) {
        if (_model->_nodes[_child_node_indices[i]].has_value()) {
            _model->_nodes[_child_node_indices[i]].value()->remove_instance(model, instances, instance_index);
        } else {
            throw std::runtime_error("error: malformed gltf: node id: " + std::to_string(_child_node_indices[i]) +
                                     " doesn't exist: " + _model->path());
        }
    }
}

void Model::preallocate(size_t count) {
    // NOTE:
    // This needs to traverse the model in the same order that write_instance_data does
    if (_scenes[_default_scene].has_value()) {
        std::vector<size_t>& root_node_indices = _scenes[_default_scene].value()->_root_node_indices;
        for (uint32_t i = 0; i < root_node_indices.size(); ++i) {
            if (_nodes[root_node_indices[i]].has_value()) {
                _nodes[root_node_indices[i]].value()->preallocate(this, count);
            } else {
                throw std::runtime_error("error: malformed gltf: node id: " + std::to_string(root_node_indices[i]) +
                                         " doesn't exist: " + path());
            }
        }
    } else {
        throw std::runtime_error("error: malformed gltf: scene id: " + std::to_string(_default_scene) + " doesn't exist: " + path());
    }
}

void Model::Node::preallocate(Model* model, size_t count) {
    if (_mesh_index.has_value()) {
        if (model->_meshes[_mesh_index.value()].has_value()) {
            std::vector<Model::Mesh::Primitive> const& primitives = model->_meshes[_mesh_index.value()].value()->_primitives;
            for (uint32_t i = 0; i < primitives.size(); ++i) {
                primitives[i]._draw->preallocate(count);
            }
        } else {
            throw std::runtime_error("error: malformed gltf: mesh id: " + std::to_string(_mesh_index.value()) +
                                     " doesn't exist: " + _model->path());
        }
    }
    for (uint32_t i = 0; i < _child_node_indices.size(); ++i) {
        if (_model->_nodes[_child_node_indices[i]].has_value()) {
            if (_model->_nodes[_child_node_indices[i]].value() == this) {
                throw std::runtime_error("node cycle detected on node: " + std::to_string(_child_node_indices[i]) +
                                         " in file: " + _model->path());
            }
            _model->_nodes[_child_node_indices[i]].value()->preallocate(model, count);
        } else {

            throw std::runtime_error("error: malformed gltf: node id: " + std::to_string(_child_node_indices[i]) +
                                     " doesn't exist: " + _model->path());
        }
    }
}

void Model::animate(std::vector<InstanceAllocPair>& instances) {
    size_t instance_index = 0;
    // NOTE:
    // This needs to traverse the model in the same order that write_instance_data does
    for (auto& root_node_index : _scenes[_default_scene].value()->_root_node_indices) {
        if (_nodes[root_node_index].has_value()) {
            _nodes[root_node_index].value()->animate(this, glm::mat4(1.0f), instances, instance_index);
        } else {
            throw std::runtime_error("error: malformed gltf: node id: " + std::to_string(root_node_index) + " doesn't exist: " + path());
        }
    }
}

void Model::Node::animate(Model* model, glm::mat4 const& parent_animation_matrix, std::vector<InstanceAllocPair>& instances,
                          size_t& instance_index) {
    if (_has_animation) {
        alignas(32) glm::mat4 animated_transform;
        fast_trs_matrix(_animation_translation, _animation_rotation, _animation_scale, _animation_matrix);
        fast_mat4_mul(parent_animation_matrix, _animation_matrix, _animation_matrix);
        fast_mat4_mul(_animation_matrix, _transform, animated_transform);
        if (_mesh_index.has_value()) {
            if (model->_meshes[_mesh_index.value()].has_value()) {
                for (uint32_t i = 0; i < model->_meshes[_mesh_index.value()].value()->_primitives.size(); ++i) {
                    instances[instance_index++].data->write(&animated_transform);
                }
            } else {
                throw std::runtime_error("error: malformed gltf: mesh id: " + std::to_string(_mesh_index.value()) +
                                         " doesn't exist: " + _model->path());
            }
        }
    } else {
        _animation_matrix = parent_animation_matrix;
    }
    for (uint32_t i = 0; i < _child_node_indices.size(); ++i) {
        if (_model->_nodes[_child_node_indices[i]].has_value()) {
            _model->_nodes[_child_node_indices[i]].value()->write_instance_data(model, _animation_matrix, instances, instance_index);
        } else {
            throw std::runtime_error("error: malformed gltf: node id: " + std::to_string(_child_node_indices[i]) +
                                     " doesn't exist: " + _model->path());
        }
    }
}

template <typename T> T lerp(T const& a, T const& b, float time) { return a + time * (b - a); }

void Model::load_animations() {
    for (fastgltf::Animation const& animation : _asset->animations) {
        for (fastgltf::AnimationChannel const& channel : animation.channels) {
            if (channel.nodeIndex.has_value()) {
                Node* node = _nodes[channel.nodeIndex.value()].value();
                if (node->_has_animation) {
                    std::cout << "warning: node id: " << channel.nodeIndex.value() << " has multiple animations on file: " << _path
                              << std::endl;
                } else {
                    _animated_nodes.push_back(node);
                }
                node->_has_animation = true;
                fastgltf::AnimationSampler const& sampler = animation.samplers[channel.samplerIndex];
                if (sampler.interpolation != fastgltf::AnimationInterpolation::Linear) {
                    std::cout << "warning: unsupported sampler interpolation" << std::endl;
                }
                // Load keyframe times from asset
                node->_animation_times = load_accessor<float>(_asset, sampler.inputAccessor);

                if (channel.path == fastgltf::AnimationPath::Translation) {
                    node->_translation_outputs = load_accessor<glm::vec3>(_asset, sampler.outputAccessor);
                    node->_has_translation_animation = true;
                } else if (channel.path == fastgltf::AnimationPath::Rotation) {
                    // NOTE:
                    // fastgltf requires vec4, and not quat,
                    // so the values need to be processed into quat
                    for (auto const& rot : load_accessor<glm::vec4>(_asset, sampler.outputAccessor)) {
                        node->_rotation_outputs.push_back(glm::make_quat(glm::value_ptr(rot)));
                    }
                    node->_has_rotation_animation = true;
                } else if (channel.path == fastgltf::AnimationPath::Scale) {
                    node->_scale_outputs = load_accessor<glm::vec3>(_asset, sampler.outputAccessor);
                    node->_has_scale_animation = true;
                } else {
                    std::cout << "warning: unsupported animation path" << std::endl;
                }
            }
        }
    }
}

// animation_current_time = now();
// animation_time_ms = animation_current_time - animation_base_time;
void Model::update_animations(uint64_t const& animation_time_ms) {
    for (Node*& node : _animated_nodes) {
        // Get the maximum keyframe time,
        // and put the animation time within the keyframe time
        uint64_t max_time_ms = 1000 * node->_animation_times.back();
        uint64_t keyframe_time_ms = animation_time_ms % max_time_ms;

        // calculate the keyframe indices
        size_t start_keyframe = 0;
        size_t end_keyframe = 0;
        for (size_t i = 0; i < node->_animation_times.size(); ++i) {
            uint64_t possible_keyframe_time_ms = 1000 * node->_animation_times[i];
            if (possible_keyframe_time_ms <= keyframe_time_ms) {
                start_keyframe = i;
            } else {
                end_keyframe = i;
                break;
            }
        }

        // calculate the time between the keyframes
        float interpolation_time = 1;
        float start_keyframe_time_ms = 1000 * node->_animation_times[start_keyframe];
        float end_keyframe_time_ms = 1000 * node->_animation_times[end_keyframe];
        if (start_keyframe != end_keyframe) {
            interpolation_time = (keyframe_time_ms - start_keyframe_time_ms) / (end_keyframe_time_ms - start_keyframe_time_ms);
        }

        // TODO:
        // support non-lerp interpolation
        // fastgltf::AnimationInterpolation const& interpolation = animation.samplers[channel.samplerIndex].interpolation;

        if (node->_has_translation_animation) {
            node->_animation_translation =
                lerp(node->_translation_outputs[start_keyframe], node->_translation_outputs[end_keyframe], interpolation_time);
        } else if (node->_has_rotation_animation) {
            node->_animation_rotation =
                glm::slerp(node->_rotation_outputs[start_keyframe], node->_rotation_outputs[end_keyframe], interpolation_time);
        } else if (node->_has_scale_animation) {
            node->_animation_scale = lerp(node->_scale_outputs[start_keyframe], node->_scale_outputs[end_keyframe], interpolation_time);
        } else {
            std::cout << "warning: unsupported animation path" << std::endl;
        }
    }
}
