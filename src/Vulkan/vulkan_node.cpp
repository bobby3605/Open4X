#include "vulkan_node.hpp"
#include "rapidjson_model.hpp"
#include <chrono>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <memory>

VulkanNode::VulkanNode(std::shared_ptr<RapidJSON_Model> model, int nodeID,
                       std::shared_ptr<std::map<int, std::shared_ptr<VulkanMesh>>> meshIDMap, std::shared_ptr<SSBOBuffers> SSBOBuffers)
    : model{model}, meshIDMap{meshIDMap}, nodeID{nodeID} {
    _baseMatrix = model->nodes[nodeID].matrix;
    if (model->nodes[nodeID].mesh.has_value()) {
        meshID = model->nodes[nodeID].mesh.value();
        // Check for unique mesh
        if (meshIDMap->count(meshID.value()) == 0) {
            // gl_BaseInstance cannot be nodeID, since only nodes with a mesh value are rendered
            meshIDMap->insert(
                {meshID.value(), std::make_shared<VulkanMesh>(model, model->nodes[nodeID].mesh.value(), SSBOBuffers->gl_BaseInstance)});
            ++SSBOBuffers->gl_BaseInstance;
        }
        // Set _modelMatrix
        _modelMatrix = &SSBOBuffers->ssboMapped[SSBOBuffers->uniqueSSBOID].modelMatrix;
        setLocationMatrix(glm::mat4(1.0f));
        //  Update instance count for each primitive
        for (std::shared_ptr<VulkanMesh::Primitive> primitive : meshIDMap->find(meshID.value())->second->primitives) {
            ++primitive->indirectDraw.instanceCount;
        }
        // Increase uniqueSSBOID
        ++SSBOBuffers->uniqueSSBOID;
        // TODO
        // What about when mesh instances do not immediately follow the mesh?
        // uniqueSSOBID may be incorrect
        // Suppose:
        // nodes: [ {mesh: 0}, {mesh: 1}, {mesh: 0, transform: [1.0, 0.0, 0.0]} ]
        // Maybe it fixes itself, since ssbo data is indexed by gl_InstanceIndex
        // trying to draw node 3 gives:
        // gl_InstanceIndex: 2
        // the vertex and index buffer is dependent on the primitive, not gl_BaseInstance
        // Might break with materials
    }
    for (int childNodeID : model->nodes[nodeID].children) {
        children.push_back(std::make_shared<VulkanNode>(model, childNodeID, meshIDMap, SSBOBuffers));
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
        std::shared_ptr<RapidJSON_Model::Animation::Channel> channel = animationPair.value().first;
        if (channel->target->node == nodeID) {
            glm::vec3 translationAnimation(1.0f);
            glm::quat rotationAnimation(1.0f, 0.0f, 0.0f, 0.0f);
            glm::vec3 scaleAnimation(1.0f);
            std::shared_ptr<RapidJSON_Model::Animation::Sampler> sampler = animationPair.value().second;
            float nowAnim =
                fmod(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count(),
                     1000 * sampler->inputData.back()) /
                1000;
            // Get the time since the past second in seconds
            // with 4 significant digits Get the animation time
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
            float interpolationValue = (nowAnim - previousTime) / (nextTime - previousTime);

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

VulkanMesh::VulkanMesh(std::shared_ptr<RapidJSON_Model> model, int meshID, int gl_BaseInstance) {
    for (RapidJSON_Model::Mesh::Primitive primitive : model->meshes[meshID].primitives) {
        primitives.push_back(std::make_shared<VulkanMesh::Primitive>(model, meshID, primitive, gl_BaseInstance));
    }
}

VulkanMesh::Primitive::Primitive(std::shared_ptr<RapidJSON_Model> model, int meshID, RapidJSON_Model::Mesh::Primitive primitive,
                                 int gl_BaseInstance) {
    RapidJSON_Model::Accessor* accessor;

    if (primitive.attributes->position.has_value()) {
        accessor = &model->accessors[primitive.attributes->position.value()];
        // Set of sparse indices
        std::set<unsigned short> sparseIndices;
        // Vector of vertices to replace with
        std::vector<glm::vec3> sparseValues;
        if (accessor->sparse.has_value()) {
            RapidJSON_Model::BufferView* bufferView;
            for (uint32_t count_index = 0; count_index < accessor->sparse->count; ++count_index) {
                // load the index
                bufferView = &model->bufferViews[accessor->sparse->indices->bufferView];
                int offset = accessor->sparse->indices->byteOffset + bufferView->byteOffset +
                             count_index * (bufferView->byteStride.has_value() ? bufferView->byteStride.value() : sizeof(unsigned short));
                sparseIndices.insert(*(reinterpret_cast<unsigned short*>(model->buffers[bufferView->buffer].data.data() + offset)));
                // load the vertex
                bufferView = &model->bufferViews[accessor->sparse->values->bufferView];
                offset = accessor->sparse->values->byteOffset + bufferView->byteOffset +
                         count_index * (bufferView->byteStride.has_value() ? bufferView->byteStride.value() : sizeof(glm::vec3));
                sparseValues.push_back(*(reinterpret_cast<glm::vec3*>(model->buffers[bufferView->buffer].data.data() + offset)));
            }
        }
        std::vector<glm::vec3>::iterator sparseValuesIterator = sparseValues.begin();

        for (uint32_t count_index = 0; count_index < accessor->count; ++count_index) {
            Vertex vertex{};
            if (sparseIndices.count(count_index) == 1) {
                vertex.pos = *sparseValuesIterator;
                ++sparseValuesIterator;
            } else {
                vertex.pos = loadAccessor<glm::vec3>(model, accessor, count_index);
            }
            vertex.texCoord = {0.0, 0.0};
            vertex.color = {1.0f, 1.0f, 1.0f};
            vertices.push_back(vertex);
        }
    }
    if (primitive.indices.has_value()) {
        accessor = &model->accessors[primitive.indices.value()];
        for (uint32_t count_index = 0; count_index < accessor->count; ++count_index) {
            indices.push_back(loadAccessor<unsigned short>(model, accessor, count_index));
        }
    }
    // TODO
    // Generate index buffer if it does not exist
    indirectDraw.indexCount = indices.size();
    indirectDraw.instanceCount = 0;
    indirectDraw.firstIndex = 0;
    indirectDraw.vertexOffset = 0;
    indirectDraw.firstInstance = gl_BaseInstance;
}
