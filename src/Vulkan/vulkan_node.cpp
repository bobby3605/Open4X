#include "vulkan_node.hpp"
#include "vulkan_image.hpp"
#include <chrono>
#include <cstdint>
#include <glm/fwd.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <iostream>
#include <memory>

VulkanNode::VulkanNode(std::shared_ptr<GLTF> model, int nodeID, std::map<int, std::shared_ptr<VulkanMesh>>* meshIDMap,
                       std::map<int, int>* materialIDMap, std::shared_ptr<SSBOBuffers> ssboBuffers)
    : model{model}, nodeID{nodeID} {
    _baseMatrix = model->nodes[nodeID].matrix;
    if (model->nodes[nodeID].mesh.has_value()) {
        meshID = model->nodes[nodeID].mesh.value();
        // Check for unique mesh
        if (meshIDMap->count(meshID.value()) == 0) {
            // gl_BaseInstance cannot be nodeID, since only nodes with a mesh value are rendered
            meshIDMap->insert(
                {meshID.value(), std::make_shared<VulkanMesh>(model, model->nodes[nodeID].mesh.value(), materialIDMap, ssboBuffers)});
        }
        // Set _modelMatrix
        _modelMatrix = &ssboBuffers->ssboMapped[ssboBuffers->uniqueObjectID].modelMatrix;
        setLocationMatrix(glm::mat4(1.0f));
        //  Update instance count for each primitive
        std::shared_ptr<VulkanMesh> mesh = meshIDMap->find(meshID.value())->second;
        int primitiveID = 0;
        for (std::shared_ptr<VulkanMesh::Primitive> primitive : mesh->primitives) {

            // -1 because gl_InstanceIndex starts at gl_BaseInstance + 0, but indirectDraw.instanceCount starts at 1
            // if indirectDraw.instanceCount == 0, then no instances are drawn
            ++primitive->indirectDraw.instanceCount;
            int currIndex = primitive->gl_BaseInstance + primitive->indirectDraw.instanceCount - 1;
            ssboBuffers->indicesMapped[currIndex].objectIndex = ssboBuffers->uniqueObjectID;
            // TODO
            // Could save memory usage by using 2 different index buffers,
            // one for per-instance data (objectIndex),
            // and another for per-primitive data (materialIndex)
            ssboBuffers->indicesMapped[currIndex].materialIndex = primitive->materialIndex;
            ++primitiveID;
        }
        ++ssboBuffers->uniqueObjectID;
    }
    for (int childNodeID : model->nodes[nodeID].children) {
        children.push_back(std::make_shared<VulkanNode>(model, childNodeID, meshIDMap, materialIDMap, ssboBuffers));
        children.back()->setLocationMatrix(_baseMatrix);
    }
}

void VulkanNode::setLocationMatrix(glm::mat4 locationMatrix) {
    // TODO
    // Clean up the logic for setting the location and updating the animation
    // There's probably a better way to do this (less matrix multiplication)
    _locationMatrix = locationMatrix;
    glm::mat4 updateMatrix = _locationMatrix * animationMatrix * _baseMatrix;
    if (_modelMatrix.has_value()) {
        *_modelMatrix.value() = updateMatrix;
    }
    for (std::shared_ptr<VulkanNode> child : children) {
        child->setLocationMatrix(updateMatrix);
    }
}

glm::mat4 const VulkanNode::modelMatrix() { return *_modelMatrix.value(); }

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
            if (_modelMatrix.has_value()) {
                *_modelMatrix.value() = _locationMatrix * animationMatrix * _baseMatrix;
            }

        } else {
            throw std::runtime_error("Animation channel target node: " + std::to_string(channel->target->node) +
                                     " does not match nodeid: " + std::to_string(nodeID));
        }
    } else {
        throw std::runtime_error("Tried to update animation on untargeted node: " + std::to_string(nodeID));
    }
}

VulkanMesh::VulkanMesh(std::shared_ptr<GLTF> model, int meshID, std::map<int, int>* materialIDMap,
                       std::shared_ptr<SSBOBuffers> ssboBuffers) {
    for (int primitiveID = 0; primitiveID < model->meshes[meshID].primitives.size(); ++primitiveID) {
        primitives.push_back(std::make_shared<VulkanMesh::Primitive>(model, meshID, primitiveID, materialIDMap, ssboBuffers));
    }
}

VulkanMesh::Primitive::Primitive(std::shared_ptr<GLTF> model, int meshID, int primitiveID, std::map<int, int>* materialIDMap,
                                 std::shared_ptr<SSBOBuffers> ssboBuffers) {
    GLTF::Accessor* accessor;
    GLTF::Mesh::Primitive* primitive = &model->meshes[meshID].primitives[primitiveID];
    gl_BaseInstance = model->primitiveBaseInstanceMap.find({model->fileNum(), meshID, primitiveID})->second;

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
    int texCoordSelector = 0;
    MaterialData materialData{};
    if (primitive->material.has_value()) {
        GLTF::Material* material = &model->materials[primitive->material.value()];
        if (materialIDMap->count(primitive->material.value()) == 0) {
            GLTF::Material::PBRMetallicRoughness* pbrMetallicRoughness = material->pbrMetallicRoughness.get();

            materialData.baseColorFactor = pbrMetallicRoughness->baseColorFactor;

            if (pbrMetallicRoughness->baseColorTexture.has_value()) {
                image =
                    std::make_shared<VulkanImage>(ssboBuffers->device, model.get(), pbrMetallicRoughness->baseColorTexture.value()->index);
                sampler = std::make_shared<VulkanSampler>(ssboBuffers->device, model.get(), model->textures[image->textureID()].sampler,
                                                          image->mipLevels());
                texCoordSelector = pbrMetallicRoughness->baseColorTexture.value()->texCoord;
            } else {
                image = std::shared_ptr<VulkanImage>(std::static_pointer_cast<VulkanImage>(ssboBuffers->defaultImage));
                sampler = std::shared_ptr<VulkanSampler>(std::static_pointer_cast<VulkanSampler>(ssboBuffers->defaultSampler));
            }
            ssboBuffers->materialMapped[ssboBuffers->uniqueMaterialID] = materialData;
            materialIDMap->insert({primitive->material.value(), ssboBuffers->uniqueMaterialID});
            materialIndex = ssboBuffers->uniqueMaterialID++;

        } else {
            materialIndex = materialIDMap->find(primitive->material.value())->second;
            image = std::shared_ptr<VulkanImage>(std::static_pointer_cast<VulkanImage>(ssboBuffers->defaultImage));
            sampler = std::shared_ptr<VulkanSampler>(std::static_pointer_cast<VulkanSampler>(ssboBuffers->defaultSampler));
        }
    } else {
        image = std::shared_ptr<VulkanImage>(std::static_pointer_cast<VulkanImage>(ssboBuffers->defaultImage));
        sampler = std::shared_ptr<VulkanSampler>(std::static_pointer_cast<VulkanSampler>(ssboBuffers->defaultSampler));
    }
    // Check for unique image and sampler
    if (ssboBuffers->uniqueImagesMap.count((void*)image.get()) == 0) {
        ssboBuffers->uniqueImagesMap.insert({(void*)image.get(), ssboBuffers->imagesCount});
        ++ssboBuffers->imagesCount;
    }
    if (ssboBuffers->uniqueSamplersMap.count((void*)sampler.get()) == 0) {
        ssboBuffers->uniqueSamplersMap.insert({(void*)sampler.get(), ssboBuffers->samplersCount});
        ++ssboBuffers->samplersCount;
    }
    ssboBuffers->materialMapped[materialIndex].imageIndex = ssboBuffers->uniqueImagesMap.find((void*)image.get())->second;
    ssboBuffers->materialMapped[materialIndex].samplerIndex = ssboBuffers->uniqueSamplersMap.find((void*)sampler.get())->second;

    // Load vertices
    if (primitive->attributes->position.has_value()) {
        accessor = &model->accessors[primitive->attributes->position.value()];
        // Set of sparse indices
        std::set<uint32_t> sparseIndices;
        // Vector of vertices to replace with
        std::vector<glm::vec3> sparseValues;
        if (accessor->sparse.has_value()) {
            GLTF::BufferView* bufferView;
            for (uint32_t count_index = 0; count_index < accessor->sparse->count; ++count_index) {
                // load the index
                bufferView = &model->bufferViews[accessor->sparse->indices->bufferView];
                int offset =
                    accessor->sparse->indices->byteOffset + bufferView->byteOffset +
                    count_index * (bufferView->byteStride.has_value() ? bufferView->byteStride.value()
                                                                      : sizeSwitch(accessor->sparse->indices->componentType, "SCALAR"));
                sparseIndices.insert(getComponent<uint32_t>(accessor->sparse->indices->componentType,
                                                            model->buffers[bufferView->buffer].data.data(), offset));
                // load the vertex
                bufferView = &model->bufferViews[accessor->sparse->values->bufferView];
                offset = accessor->sparse->values->byteOffset + bufferView->byteOffset +
                         count_index * (bufferView->byteStride.has_value() ? bufferView->byteStride.value()
                                                                           : sizeSwitch(accessor->componentType, accessor->type));
                sparseValues.push_back(
                    getComponent<glm::vec3>(accessor->componentType, model->buffers[bufferView->buffer].data.data(), offset));
            }
        }
        std::vector<glm::vec3>::iterator sparseValuesIterator = sparseValues.begin();

        std::unordered_map<Vertex, uint32_t> uniqueVertices{};
        for (uint32_t count_index = 0; count_index < accessor->count; ++count_index) {
            Vertex vertex{};
            if (sparseIndices.count(count_index) == 1) {
                vertex.pos = *sparseValuesIterator;
                ++sparseValuesIterator;
            } else {
                vertex.pos = loadAccessor<glm::vec3>(model, accessor, count_index);
            }

            vertex.texCoord = {0.0f, 0.0f};
            if (primitive->attributes->texcoords.size() > 0) {
                for (int texCoordSelected = 0; texCoordSelected < primitive->attributes->texcoords.size(); ++texCoordSelected) {
                    if (texCoordSelected == texCoordSelector) {
                        vertex.texCoord = loadAccessor<glm::vec2>(
                            model, &model->accessors[primitive->attributes->texcoords[texCoordSelected]], count_index);
                        break;
                    }
                }
            }

            vertices.push_back(vertex);
            // Generate indices if none exist
            if (!primitive->indices.has_value()) {
                if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }
                indices.push_back(uniqueVertices[vertex]);
            }
        }
    }
    if (primitive->indices.has_value()) {
        accessor = &model->accessors[primitive->indices.value()];
        for (uint32_t count_index = 0; count_index < accessor->count; ++count_index) {
            indices.push_back(loadAccessor<uint32_t>(model, accessor, count_index));
        }
    }

    indirectDraw.indexCount = indices.size();
    indirectDraw.instanceCount = 0;
    indirectDraw.firstIndex = 0;
    indirectDraw.vertexOffset = 0;
    indirectDraw.firstInstance = gl_BaseInstance;
}
