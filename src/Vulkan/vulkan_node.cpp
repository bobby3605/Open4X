#include "vulkan_node.hpp"
#include "aabb.hpp"
#include "vulkan_buffer.hpp"
#include "vulkan_image.hpp"
#include <chrono>
#include <cstdint>
#include <glm/fwd.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <iostream>
#include <memory>
#include <stdexcept>

VulkanNode::VulkanNode(std::shared_ptr<GLTF> model, int nodeID, std::unordered_map<int, std::shared_ptr<VulkanMesh>>* meshIDMap,
                       std::unordered_map<int, int>* materialIDMap, uint32_t& instanceCounter, std::shared_ptr<SSBOBuffers> ssboBuffers)
    : model{model}, nodeID{nodeID} {
    _baseMatrix = &model->nodes[nodeID].matrix;
    if (model->nodes[nodeID].mesh.has_value()) {
        meshID = model->nodes[nodeID].mesh.value();
        if (meshIDMap->count(meshID) == 0) {
            // gl_BaseInstance cannot be nodeID, since only nodes with a mesh value are rendered
            meshIDMap->insert(
                {meshID, std::make_shared<VulkanMesh>(model.get(), model->nodes[nodeID].mesh.value(), materialIDMap, ssboBuffers)});
        }
        // Update instance count for each primitive
        mesh = meshIDMap->find(meshID)->second;
        // NOTE:
        // Every node has a matrix
        // Not every node has a mesh
        // Only nodes with a mesh need to be uploaded
        // So only nodes with a mesh get an instance id
        ++instanceCounter;
    }
    children.reserve(model->nodes[nodeID].children.size());
    for (int childNodeID : model->nodes[nodeID].children) {
        children.push_back(new VulkanNode(model, childNodeID, meshIDMap, materialIDMap, instanceCounter, ssboBuffers));
    }
}

void VulkanNode::addInstance(uint32_t& globalInstanceIDIterator, std::shared_ptr<SSBOBuffers> ssboBuffers) {
    if (mesh != nullptr) {
        // TODO:
        // Get rid of the mutex
        mesh->instanceIDsMutex.lock();
        mesh->instanceIDs.push_back(globalInstanceIDIterator++);
        mesh->instanceIDsMutex.unlock();
    }
    for (const auto child : children) {
        child->addInstance(globalInstanceIDIterator, ssboBuffers);
    }
}

void VulkanNode::updateAABB(glm::mat4 parentMatrix, AABB& aabb) {
    glm::mat4 modelMatrix = parentMatrix * *_baseMatrix;
    if (mesh != nullptr) {
        aabb.update(modelMatrix * glm::vec4(mesh->aabb.max(), 1.0f));
        aabb.update(modelMatrix * glm::vec4(mesh->aabb.min(), 1.0f));
    }
    for (const auto child : children) {
        child->updateAABB(modelMatrix, aabb);
    }
}

VulkanNode::~VulkanNode() {
    for (auto child : children) {
        delete child;
    }
}

void VulkanNode::uploadModelMatrix(uint32_t& globalInstanceID, glm::mat4 parentMatrix, std::shared_ptr<SSBOBuffers> ssboBuffers) {
    glm::mat4 modelMatrix{1.0f};
    if (animationPair.has_value()) {
        modelMatrix = parentMatrix = parentMatrix * animationMatrix * *_baseMatrix;
    } else {
        modelMatrix = parentMatrix = parentMatrix * *_baseMatrix;
    }
    if (mesh != nullptr) {
        // Decompose matrix
        // https://math.stackexchange.com/a/1463487
        glm::vec3 translation((modelMatrix)[3]);
        (modelMatrix)[3] = glm::vec4(0, 0, 0, 1);

        (modelMatrix)[0][3] = 0;
        float scalex = glm::length((modelMatrix)[0]);
        (modelMatrix)[1][3] = 0;
        float scaley = glm::length((modelMatrix)[1]);
        (modelMatrix)[2][3] = 0;
        float scalez = glm::length((modelMatrix)[2]);
        glm::vec3 scale(scalex, scaley, scalez);

        (modelMatrix)[0] /= scalex;
        (modelMatrix)[1] /= scaley;
        (modelMatrix)[2] /= scalez;
        glm::quat rotation = glm::toQuat(modelMatrix);

        ssboBuffers->ssboMapped[globalInstanceID].translation = translation;
        ssboBuffers->ssboMapped[globalInstanceID].rotation = rotation;
        ssboBuffers->ssboMapped[globalInstanceID].scale = scale;

        ++globalInstanceID;
    }

    for (const auto child : children) {
        // NOTE:
        // Parent matrix was updated at the top of the function
        child->uploadModelMatrix(globalInstanceID, parentMatrix, ssboBuffers);
    }
}

void VulkanNode::updateAnimation() {
    if (animationPair.has_value()) {
        std::shared_ptr<GLTF::Animation::Channel> channel = animationPair.value().first;
        if (channel->target->node == nodeID) {
            glm::vec3 translationAnimation(1.0f);
            glm::quat rotationAnimation(1.0f, 0.0f, 0.0f, 0.0f);
            glm::vec3 scaleAnimation(1.0f);
            std::shared_ptr<GLTF::Animation::Sampler> sampler = animationPair.value().second;
            // Get the time since the past second in seconds
            // with 4 significant digits Get the animation time
            float nowAnim =
                fmod(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count(),
                     1000 * sampler->inputData.back()) /
                1000;
            glm::mat4 lerp1 = sampler->outputData.front();
            glm::mat4 lerp2 = sampler->outputData.front();
            float previousTime = 0;
            float nextTime = 0;
            for (int i = 0; i < sampler->inputData.size(); ++i) {
                if (sampler->inputData[i] <= nowAnim) {
                    previousTime = sampler->inputData[i];
                    lerp1 = sampler->outputData[i];
                }
                if (sampler->inputData[i] >= nowAnim) {
                    nextTime = sampler->inputData[i];
                    lerp2 = sampler->outputData[i];
                    break;
                }
            }
            float interpolationValue = 1;
            if (nextTime != previousTime) {
                interpolationValue = (nowAnim - previousTime) / (nextTime - previousTime);
            }
            if (channel->target->path.compare("translation") == 0) {

                translationAnimation = glm::make_vec3(glm::value_ptr(lerp1)) +
                                       interpolationValue * (glm::make_vec3(glm::value_ptr(lerp2)) - glm::make_vec3(glm::value_ptr(lerp1)));

            } else if (channel->target->path.compare("rotation") == 0) {

                rotationAnimation =
                    glm::slerp(glm::make_quat(glm::value_ptr(lerp1)), glm::make_quat(glm::value_ptr(lerp2)), interpolationValue);

            } else if (channel->target->path.compare("scale") == 0) {
                scaleAnimation = glm::make_vec3(glm::value_ptr(lerp1)) +
                                 interpolationValue * (glm::make_vec3(glm::value_ptr(lerp2)) - glm::make_vec3(glm::value_ptr(lerp1)));

            } else {
                std::cout << "Unknown animationChannel type: " << channel->target->path << std::endl;
            }

            glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), translationAnimation);

            glm::mat4 rotationMatrix = glm::toMat4(rotationAnimation);

            glm::mat4 scaleMatrix = glm::scale(scaleAnimation);

            animationMatrix = translationMatrix * rotationMatrix * scaleMatrix;

        } else {
            throw std::runtime_error("Animation channel target node: " + std::to_string(channel->target->node) +
                                     " does not match nodeid: " + std::to_string(nodeID));
        }
    } else {
        throw std::runtime_error("Tried to update animation on untargeted node: " + std::to_string(nodeID));
    }
}

VulkanMesh::VulkanMesh(GLTF* model, uint32_t meshID, std::unordered_map<int, int>* materialIDMap, std::shared_ptr<SSBOBuffers> ssboBuffers)
    : _meshID{meshID} {
    for (int primitiveID = 0; primitiveID < model->meshes[meshID].primitives.size(); ++primitiveID) {
        primitives.push_back(std::make_shared<VulkanMesh::Primitive>(model, meshID, primitiveID, materialIDMap, ssboBuffers));
        aabb.update(primitives.back()->aabb);
    }
}

VulkanMesh::Primitive::Primitive(GLTF* model, int meshID, int primitiveID, std::unordered_map<int, int>* materialIDMap,
                                 std::shared_ptr<SSBOBuffers> ssboBuffers) {

    GLTF::Accessor* accessor;
    GLTF::Mesh::Primitive* primitive = &model->meshes[meshID].primitives[primitiveID];

    // Load unique materials
    //
    // If material has not been registered,
    // create it,
    // then add its index the id map,
    // then set materialIndex to its index and increment the unique id count
    // If it has been registered,
    // get the material buffer index from the map and set materialIndex to it
    //
    // If it doesn't have a material, then leave materialIndex at 0, which is the default material
    // NOTE:
    // might need separate texCoordSelector and texCoord for normal map
    int texCoordSelector = 0;
    if (primitive->material.has_value()) {
        GLTF::Material* material = &model->materials[primitive->material.value()];
        // NOTE:
        // This might need a mutex
        if (materialIDMap->count(primitive->material.value()) == 0) {
            unique = 1;
            GLTF::Material::PBRMetallicRoughness* pbrMetallicRoughness = material->pbrMetallicRoughness.get();

            materialData.baseColorFactor = pbrMetallicRoughness->baseColorFactor;

            if (pbrMetallicRoughness->baseColorTexture.has_value()) {
                image = std::make_shared<VulkanImage>(ssboBuffers->device, model, pbrMetallicRoughness->baseColorTexture.value()->index);
                if (model->textures[image->textureID()].sampler.has_value()) {
                    sampler = std::make_shared<VulkanSampler>(ssboBuffers->device, model,
                                                              model->textures[image->textureID()].sampler.value(), image->mipLevels());
                } else {

                    sampler = std::shared_ptr<VulkanSampler>(std::static_pointer_cast<VulkanSampler>(ssboBuffers->defaultSampler));
                }
                texCoordSelector = pbrMetallicRoughness->baseColorTexture.value()->texCoord;
            } else {
                image = std::shared_ptr<VulkanImage>(std::static_pointer_cast<VulkanImage>(ssboBuffers->defaultImage));
                sampler = std::shared_ptr<VulkanSampler>(std::static_pointer_cast<VulkanSampler>(ssboBuffers->defaultSampler));
            }

            if (pbrMetallicRoughness->metallicRoughnessTexture.has_value()) {
                metallicRoughnessMap = std::make_shared<VulkanImage>(
                    ssboBuffers->device, model, pbrMetallicRoughness->metallicRoughnessTexture.value()->index, VK_FORMAT_R8G8B8A8_UNORM);
                metallicFactor = pbrMetallicRoughness->metallicFactor;
                roughnessFactor = pbrMetallicRoughness->roughnessFactor;
            } else {
                metallicRoughnessMap =
                    std::shared_ptr<VulkanImage>(std::static_pointer_cast<VulkanImage>(ssboBuffers->defaultMetallicRoughnessMap));
            }

            if (material->normalTexture.has_value()) {
                normalMap = std::make_shared<VulkanImage>(ssboBuffers->device, model, material->normalTexture.value()->index,
                                                          VK_FORMAT_R8G8B8A8_UNORM);
                normalScale = material->normalTexture.value()->scale;
            } else {
                normalMap = std::shared_ptr<VulkanImage>(std::static_pointer_cast<VulkanImage>(ssboBuffers->defaultNormalMap));
            }

            if (material->occlusionTexture.has_value()) {
                aoMap = std::make_shared<VulkanImage>(ssboBuffers->device, model, material->occlusionTexture.value()->index,
                                                      VK_FORMAT_R8G8B8A8_UNORM);
                occlusionStrength = material->occlusionTexture.value()->scale;
            } else {
                aoMap = std::shared_ptr<VulkanImage>(std::static_pointer_cast<VulkanImage>(ssboBuffers->defaultAoMap));
            }

            materialIndex = ssboBuffers->uniqueMaterialID++;
            materialIDMap->insert({primitive->material.value(), materialIndex});

        } else {
            materialIndex = materialIDMap->find(primitive->material.value())->second;
            image = std::shared_ptr<VulkanImage>(std::static_pointer_cast<VulkanImage>(ssboBuffers->defaultImage));
            sampler = std::shared_ptr<VulkanSampler>(std::static_pointer_cast<VulkanSampler>(ssboBuffers->defaultSampler));
            metallicRoughnessMap =
                std::shared_ptr<VulkanImage>(std::static_pointer_cast<VulkanImage>(ssboBuffers->defaultMetallicRoughnessMap));
            normalMap = std::shared_ptr<VulkanImage>(std::static_pointer_cast<VulkanImage>(ssboBuffers->defaultNormalMap));
            aoMap = std::shared_ptr<VulkanImage>(std::static_pointer_cast<VulkanImage>(ssboBuffers->defaultAoMap));
        }
    } else {
        image = std::shared_ptr<VulkanImage>(std::static_pointer_cast<VulkanImage>(ssboBuffers->defaultImage));
        sampler = std::shared_ptr<VulkanSampler>(std::static_pointer_cast<VulkanSampler>(ssboBuffers->defaultSampler));
        metallicRoughnessMap =
            std::shared_ptr<VulkanImage>(std::static_pointer_cast<VulkanImage>(ssboBuffers->defaultMetallicRoughnessMap));
        normalMap = std::shared_ptr<VulkanImage>(std::static_pointer_cast<VulkanImage>(ssboBuffers->defaultNormalMap));
        aoMap = std::shared_ptr<VulkanImage>(std::static_pointer_cast<VulkanImage>(ssboBuffers->defaultAoMap));
    }
    if (unique) {
        // Check for unique maps
        if (ssboBuffers->uniqueImagesMap.count((void*)image.get()) == 0) {
            ssboBuffers->uniqueImagesMap.insert({(void*)image.get(), ssboBuffers->imagesCount.fetch_add(1, std::memory_order_relaxed)});
        }
        if (ssboBuffers->uniqueSamplersMap.count((void*)sampler.get()) == 0) {
            ssboBuffers->uniqueSamplersMap.insert(
                {(void*)sampler.get(), ssboBuffers->samplersCount.fetch_add(1, std::memory_order_relaxed)});
        }
        if (ssboBuffers->uniqueMetallicRoughnessMapsMap.count((void*)metallicRoughnessMap.get()) == 0) {
            ssboBuffers->uniqueMetallicRoughnessMapsMap.insert(
                {(void*)metallicRoughnessMap.get(), ssboBuffers->metallicRoughnessMapsCount.fetch_add(1, std::memory_order_relaxed)});
        }
        if (ssboBuffers->uniqueNormalMapsMap.count((void*)normalMap.get()) == 0) {
            ssboBuffers->uniqueNormalMapsMap.insert(
                {(void*)normalMap.get(), ssboBuffers->normalMapsCount.fetch_add(1, std::memory_order_relaxed)});
        }
        if (ssboBuffers->uniqueAoMapsMap.count((void*)aoMap.get()) == 0) {
            ssboBuffers->uniqueAoMapsMap.insert({(void*)aoMap.get(), ssboBuffers->aoMapsCount.fetch_add(1, std::memory_order_relaxed)});
        }
    }

    // Load vertices
    GLTF::Mesh::Primitive::Attributes* attributes = primitive->attributes.get();
    if (attributes->position.has_value()) {
        accessor = &model->accessors[attributes->position.value()];
        // Set of sparse indices
        std::set<uint32_t> sparseIndices;
        // Vector of vertices to replace with
        std::vector<glm::vec3> sparseValues;
        if (accessor->sparse.has_value()) {
            AccessorLoader<uint32_t> sparseIndicesLoader(model, accessor, &model->bufferViews[accessor->sparse->indices->bufferView],
                                                         accessor->sparse->indices->byteOffset, accessor->sparse->indices->componentType,
                                                         "SCALAR");
            AccessorLoader<glm::vec3> sparseValuesLoader(model, accessor, &model->bufferViews[accessor->sparse->values->bufferView],
                                                         accessor->sparse->values->byteOffset, accessor->componentType, accessor->type);
            sparseValues.reserve(accessor->sparse->count);
            for (uint32_t count_index = 0; count_index < accessor->sparse->count; ++count_index) {
                // load the index
                sparseIndices.insert(sparseIndicesLoader.at(count_index));
                // load the vertex
                sparseValues.push_back(sparseValuesLoader.at(count_index));
            }
        }
        std::vector<glm::vec3>::iterator sparseValuesIterator = sparseValues.begin();

        std::unordered_map<Vertex, uint32_t> uniqueVertices{};

        // Create accessor loaders for each attribute
        AccessorLoader<glm::vec3> positionAccessor(model, accessor);
        AccessorLoader<glm::vec2>* texCoordAccessor = nullptr;
        if (attributes->texcoords.size() > 0) {
            texCoordAccessor = new AccessorLoader<glm::vec2>(model, &model->accessors[attributes->texcoords[texCoordSelector]]);
        }
        AccessorLoader<glm::vec3>* normalAccessor = nullptr;
        if (attributes->normal.has_value()) {
            normalAccessor = new AccessorLoader<glm::vec3>(model, &model->accessors[attributes->normal.value()]);
        }

        vertices.resize(accessor->count);
        for (uint32_t count_index = 0; count_index < accessor->count; ++count_index) {
            Vertex vertex{};
            if (sparseIndices.size() != 0 && sparseIndices.count(count_index) == 1) {
                vertex.pos = *sparseValuesIterator;
                ++sparseValuesIterator;
            } else {
                vertex.pos = positionAccessor.at(count_index);
            }

            aabb.update(vertex.pos);

            vertex.texCoord = {0.0f, 0.0f};
            // Texcoord
            if (attributes->texcoords.size() > 0) {
                vertex.texCoord = texCoordAccessor->at(count_index);
            }

            // Normal
            vertex.normal = {0.0f, 0.0f, 1.0f};
            if (attributes->normal.has_value()) {
                vertex.normal = normalAccessor->at(count_index);
            }

            vertex.tangent = {0.0f, 1.0f, 0.0f, 1.0f};
            if (attributes->tangent.has_value()) {
                // vertex.tangent = loadAccessor<glm::vec4>(model, &model->accessors[attributes->tangent.value()], count_index);
            }

            vertices[count_index] = vertex;
            // Generate indices if none exist
            if (!primitive->indices.has_value()) {
                if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }
                indices.push_back(uniqueVertices[vertex]);
            }
        }
        if (texCoordAccessor != nullptr) {
            delete texCoordAccessor;
        }
        if (normalAccessor != nullptr) {
            delete normalAccessor;
        }
    }

    // NOTE:
    // Not currently used because the fragment shader calculates tangents
    // Calculate tangents
    // NOTE:
    // possibly calculate tangents in only one pass
    // NOTE:
    // doesn't work
    if (!attributes->tangent.has_value() && 0) {
        for (int i = 0; i < vertices.size(); i += 3) {

            Vertex vertex = vertices[i];

            glm::vec3 pos1 = vertex.pos;
            glm::vec3 pos2 = vertices[i + 1].pos;
            glm::vec3 pos3 = vertices[i + 2].pos;
            glm::vec2 uv1 = vertices[i].texCoord;
            glm::vec2 uv2 = vertices[i + 1].texCoord;
            glm::vec2 uv3 = vertices[i + 2].texCoord;

            // https://learnopengl.com/Advanced-Lighting/Normal-Mapping
            glm::vec3 edge1 = pos2 - pos1;
            glm::vec3 edge2 = pos3 - pos1;
            glm::vec2 deltaUV1 = uv2 - uv1;
            glm::vec2 deltaUV2 = uv3 - uv1;
            float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

            vertices[i].tangent[0] = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
            vertices[i].tangent[1] = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
            vertices[i].tangent[2] = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
            vertices[i].tangent[3] = 1.0f;
            /*
                        vertices[i + 1].tangent[0] = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
                        vertices[i + 1].tangent[1] = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
                        vertices[i + 1].tangent[2] = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
                        vertices[i + 1].tangent[3] = 1.0f;

                        vertices[i + 2].tangent[0] = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
                        vertices[i + 2].tangent[1] = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
                        vertices[i + 2].tangent[2] = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
                        vertices[i + 2].tangent[3] = 1.0f;
                        */
        }
    }
    if (primitive->indices.has_value()) {
        accessor = &model->accessors[primitive->indices.value()];
        indices.resize(accessor->count);
        AccessorLoader<uint32_t> indicesAccessor(model, accessor);
        for (uint32_t count_index = 0; count_index < accessor->count; ++count_index) {
            indices[count_index] = indicesAccessor.at(count_index);
        }
    }
}

void VulkanMesh::Primitive::uploadMaterial(std::shared_ptr<SSBOBuffers> ssboBuffers) {
    if (unique) {
        ssboBuffers->materialMapped[materialIndex] = materialData;
        ssboBuffers->materialMapped[materialIndex].imageIndex = ssboBuffers->uniqueImagesMap.find((void*)image.get())->second;
        ssboBuffers->materialMapped[materialIndex].samplerIndex = ssboBuffers->uniqueSamplersMap.find((void*)sampler.get())->second;
        ssboBuffers->materialMapped[materialIndex].metallicRoughnessMapIndex =
            ssboBuffers->uniqueMetallicRoughnessMapsMap.find((void*)metallicRoughnessMap.get())->second;
        ssboBuffers->materialMapped[materialIndex].metallicFactor = metallicFactor;
        ssboBuffers->materialMapped[materialIndex].roughnessFactor = roughnessFactor;
        ssboBuffers->materialMapped[materialIndex].normalMapIndex = ssboBuffers->uniqueNormalMapsMap.find((void*)normalMap.get())->second;
        ssboBuffers->materialMapped[materialIndex].normalScale = normalScale;
        ssboBuffers->materialMapped[materialIndex].aoMapIndex = ssboBuffers->uniqueAoMapsMap.find((void*)aoMap.get())->second;
        ssboBuffers->materialMapped[materialIndex].occlusionStrength = occlusionStrength;
    }
}
