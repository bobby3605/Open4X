#include "vulkan_object.hpp"
#include "rapidjson_model.hpp"
#include "vulkan_renderer.hpp"
#include <chrono>
#include <cstdint>
#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/transform.hpp>
#include <iostream>
#include <vulkan/vulkan_core.h>

VulkanObject::VulkanObject(RapidJSON_Model* model) : model{model} {
    for (RapidJSON_Model::Scene scene : model->scenes) {
        for (int nodeID : scene.nodes) {
            RapidJSON_Model::Node node = model->nodes[nodeID];
            _nodeMatrices.push_back(node.matrix);
            if (node.mesh.has_value()) {
                loadMesh(nodeID, nodeID);
            } else if (node.children.size() > 0) {
                // TODO
                // might fail with multiple meshes and instances,
                // since gl_BaseInstance is the nodeID
                for (int child : node.children) {
                    loadMesh(model->nodes[child].mesh.value(), nodeID);
                }
                continue;
            } else {
                throw std::runtime_error("no mesh found on node: " + std::to_string(nodeID));
            }
        }
    }
    // Load animation data
    if (model->animations.size() > 0) {
        _hasAnimation = 1;
        for (RapidJSON_Model::Animation animation : model->animations) {
            for (std::shared_ptr<RapidJSON_Model::Animation::Sampler> sampler : animation.samplers) {
                RapidJSON_Model::Accessor inputAccessor = model->accessors[sampler->inputIndex];
                for (int i = 0; i < inputAccessor.count; ++i) {
                    sampler->inputData.push_back(loadAccessor<float>(&inputAccessor, i));
                }
                RapidJSON_Model::Accessor outputAccessor = model->accessors[sampler->outputIndex];
                for (int i = 0; i < outputAccessor.count; ++i) {
                    sampler->outputData.push_back(glm::make_mat4(glm::value_ptr(loadAccessor<glm::vec4>(&outputAccessor, i))));
                }
            }
        }
    }
}

void VulkanObject::loadMesh(int meshID, int nodeID) {
    RapidJSON_Model::Mesh mesh = model->meshes[meshID];
    for (RapidJSON_Model::Mesh::Primitive primitive : mesh.primitives) {
        RapidJSON_Model::Accessor* accessor;
        int vertexOffset = 0;
        int firstIndex = 0;
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
                    int offset =
                        accessor->sparse->indices->byteOffset + bufferView->byteOffset +
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
                    vertex.pos = loadAccessor<glm::vec3>(accessor, count_index);
                }
                vertex.texCoord = {0.0, 0.0};
                vertex.color = {1.0f, 1.0f, 1.0f};
                vertices.push_back(vertex);
            }
        }
        if (primitive.indices.has_value()) {
            accessor = &model->accessors[primitive.indices.value()];
            for (uint32_t count_index = 0; count_index < accessor->count; ++count_index) {
                indices.push_back(loadAccessor<unsigned short>(accessor, count_index));
            }
        }
        // TODO
        // Generate index buffer if it does not exist
        // Add instancing for duplicate meshes
        VkDrawIndexedIndirectCommand indirectDraw{};
        indirectDraw.indexCount = indices.size() - firstIndex;
        indirectDraw.instanceCount = 1;
        indirectDraw.firstIndex = firstIndex;
        indirectDraw.vertexOffset = vertexOffset;
        indirectDraw.firstInstance = nodeID;
        indirectDraws.push_back(indirectDraw);

        firstIndex = indices.size();
        vertexOffset = vertices.size();
    }
}

VulkanObject::VulkanObject() {}

void VulkanObject::setPostion(glm::vec3 newPosition) {
    _position = newPosition;
    updateModelMatrix();
}
void VulkanObject::setRotation(glm::quat newRotation) {
    _rotation = newRotation;
    updateModelMatrix();
}
void VulkanObject::setScale(glm::vec3 newScale) {
    _scale = newScale;
    updateModelMatrix();
}

void VulkanObject::x(float newX) {
    _position.x = newX;
    updateModelMatrix();
}
void VulkanObject::y(float newY) {
    _position.y = newY;
    updateModelMatrix();
}
void VulkanObject::z(float newZ) {
    _position.z = newZ;
    updateModelMatrix();
}

void VulkanObject::updateModelMatrix() {
    _modelMatrix = glm::translate(glm::mat4(1.0f), position()) * glm::toMat4(rotation()) * glm::scale(scale());
}

glm::mat4 const VulkanObject::nodeModelMatrix(int nodeID) {
    if (hasAnimation()) {
        return _modelMatrix * getAnimation(nodeID) * model->nodes[nodeID].matrix;
    } else {
        return _modelMatrix * model->nodes[nodeID].matrix;
    }
}

glm::mat4 const VulkanObject::getAnimation(int nodeID) {
    for (RapidJSON_Model::Animation animation : model->animations) {
        for (std::shared_ptr<RapidJSON_Model::Animation::Channel> channel : animation.channels) {
            if (channel->target->node == nodeID) {
                glm::vec3 translationAnimation(1.0f);
                glm::quat rotationAnimation(1.0f, 0.0f, 0.0f, 0.0f);
                glm::vec3 scaleAnimation(1.0f);
                std::shared_ptr<RapidJSON_Model::Animation::Sampler> sampler = animation.samplers[channel->sampler];
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

                    translationAnimation =
                        glm::make_vec3(glm::value_ptr(lerp1)) +
                        interpolationValue * (glm::make_vec3(glm::value_ptr(lerp2)) - glm::make_vec3(glm::value_ptr(lerp1)));

                    std::cout << "translationAnimation: " << glm::to_string(translationAnimation) << std::endl;

                } else if (channel->target->path.compare("rotation") == 0) {

                    rotationAnimation =
                        glm::slerp(glm::make_quat(glm::value_ptr(lerp1)), glm::make_quat(glm::value_ptr(lerp2)), interpolationValue);

                } else if (channel->target->path.compare("scale") == 0) {
                    scaleAnimation = glm::make_vec3(glm::value_ptr(lerp1)) +
                                     interpolationValue * (glm::make_vec3(glm::value_ptr(lerp2)) - glm::make_vec3(glm::value_ptr(lerp1)));

                } else {
                    std::cout << "Unknown channel type: " << channel->target->path << std::endl;
                }

                glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), translationAnimation * position());

                glm::mat4 rotationMatrix = glm::toMat4(rotationAnimation * rotation());

                glm::mat4 scaleMatrix = glm::scale(scaleAnimation * scale());

                return translationMatrix * rotationMatrix * scaleMatrix;
            }
        }
    }
    return glm::mat4(1.0f);
}

// TODO
// Take glfwWindow instead of renderer
void VulkanObject::keyboardUpdate(VulkanRenderer* renderer, float frameTime) {
    glm::vec3 rotate{0};
    float speedUp = 1;
    if (glfwGetKey(renderer->getWindow()->getGLFWwindow(), keys.yawRight) == GLFW_PRESS)
        rotate.y -= 1.f;
    if (glfwGetKey(renderer->getWindow()->getGLFWwindow(), keys.yawLeft) == GLFW_PRESS)
        rotate.y += 1.f;
    if (glfwGetKey(renderer->getWindow()->getGLFWwindow(), keys.pitchUp) == GLFW_PRESS)
        rotate.x += 1.f;
    if (glfwGetKey(renderer->getWindow()->getGLFWwindow(), keys.pitchDown) == GLFW_PRESS)
        rotate.x -= 1.f;
    if (glfwGetKey(renderer->getWindow()->getGLFWwindow(), keys.rollLeft) == GLFW_PRESS)
        rotate.z += 1.f;
    if (glfwGetKey(renderer->getWindow()->getGLFWwindow(), keys.rollRight) == GLFW_PRESS)
        rotate.z -= 1.f;
    if (glfwGetKey(renderer->getWindow()->getGLFWwindow(), keys.speedUp) == GLFW_PRESS)
        speedUp = 2;

    if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon()) {
        setRotation(rotation() * glm::quat(speedUp * lookSpeed * frameTime * glm::normalize(rotate)));
    }

    const glm::vec3 forwardDir = rotation() * glm::vec3(0.0f, 0.0f, -1.0f);
    const glm::vec3 rightDir = rotation() * glm::vec3(-1.0f, 0.0, 0.0f);
    const glm::vec3 upDir = rotation() * glm::vec3(0.f, -1.0f, 0.f);

    glm::vec3 moveDir{0.f};
    speedUp = 1;
    if (glfwGetKey(renderer->getWindow()->getGLFWwindow(), keys.moveForward) == GLFW_PRESS)
        moveDir += forwardDir;
    if (glfwGetKey(renderer->getWindow()->getGLFWwindow(), keys.moveBackward) == GLFW_PRESS)
        moveDir -= forwardDir;
    if (glfwGetKey(renderer->getWindow()->getGLFWwindow(), keys.moveRight) == GLFW_PRESS)
        moveDir += rightDir;
    if (glfwGetKey(renderer->getWindow()->getGLFWwindow(), keys.moveLeft) == GLFW_PRESS)
        moveDir -= rightDir;
    if (glfwGetKey(renderer->getWindow()->getGLFWwindow(), keys.moveUp) == GLFW_PRESS)
        moveDir += upDir;
    if (glfwGetKey(renderer->getWindow()->getGLFWwindow(), keys.moveDown) == GLFW_PRESS)
        moveDir -= upDir;
    if (glfwGetKey(renderer->getWindow()->getGLFWwindow(), keys.speedUp) == GLFW_PRESS)
        speedUp = 5;

    if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) {
        setPostion(position() + (speedUp * moveSpeed * frameTime * glm::normalize(moveDir)));
    }

    if (glfwGetKey(renderer->getWindow()->getGLFWwindow(), GLFW_KEY_SPACE))
        std::cout << "Postion: " << glm::to_string(position()) << std::endl << "Rotation: " << glm::to_string(rotation()) << std::endl;
}
