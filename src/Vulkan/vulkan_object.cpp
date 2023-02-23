#include "vulkan_object.hpp"
#include "../glTF/AccessorLoader.hpp"
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

VulkanObject::VulkanObject(std::shared_ptr<VulkanModel> vulkanModel, std::shared_ptr<SSBOBuffers> ssboBuffers, std::string const& name,
                           bool duplicate)
    : model{vulkanModel->model} {
    _name = new char[name.size()];
    for (int i = 0; i < name.size(); ++i) {
        _name[i] = name[i];
    }
    // Load nodes and meshes
    for (int sceneIndex = 0; sceneIndex < model->scenes.size(); ++sceneIndex) {
        rootNodes.reserve(rootNodes.size() + model->scenes[sceneIndex].nodes.size());
        for (int rootNodeID : model->scenes[sceneIndex].nodes) {
            rootNodes.push_back(
                new VulkanNode(model, rootNodeID, &vulkanModel->meshIDMap, &vulkanModel->materialIDMap, ssboBuffers, duplicate));
        }
        // Load animation data
        for (GLTF::Animation animation : model->animations) {
            if (animatedNodes == nullptr) {
                animatedNodes = new std::vector<VulkanNode*>;
            }
            for (std::shared_ptr<GLTF::Animation::Channel> channel : animation.channels) {
                std::shared_ptr<GLTF::Animation::Sampler> sampler = animation.samplers[channel->sampler];
                std::optional<VulkanNode*> node = findNode(channel->target->node);
                if (node.has_value()) {
                    animatedNodes->push_back(node.value());
                    node.value()->animationPair = {channel, sampler};
                    node.value()->animationMatrix = new glm::mat4(1.0f);
                    node.value()->_locationMatrix = new glm::mat4(1.0f);
                }
                GLTF::Accessor* inputAccessor = &model->accessors[sampler->inputIndex];
                AccessorLoader<float> inputAccessorLoader(model.get(), inputAccessor);
                for (int count_index = 0; count_index < inputAccessor->count; ++count_index) {
                    sampler->inputData.push_back(inputAccessorLoader.at(count_index));
                }
                GLTF::Accessor* outputAccessor = &model->accessors[sampler->outputIndex];
                AccessorLoader<glm::vec4> outputAccessorLoader(model.get(), outputAccessor);
                for (int count_index = 0; count_index < outputAccessor->count; ++count_index) {
                    sampler->outputData.push_back(glm::make_mat4(glm::value_ptr(outputAccessorLoader.at(count_index))));
                }
            }
        }
    }
}

VulkanObject::~VulkanObject() {
    for (auto node : rootNodes) {
        delete node;
    }
    if (animatedNodes != nullptr) {
        delete animatedNodes;
    }
    if (_name != nullptr) {
        delete _name;
    }
}

void VulkanObject::updateAnimations(std::shared_ptr<SSBOBuffers> ssboBuffers) {
    for (VulkanNode* node : *animatedNodes) {
        node->updateAnimation();
        node->uploadModelMatrix(ssboBuffers);
    }
}

void VulkanObject::uploadModelMatrices(std::shared_ptr<SSBOBuffers> ssboBuffers) {
    for (VulkanNode* node : rootNodes) {
        node->uploadModelMatrix(ssboBuffers);
    }
}

std::optional<VulkanNode*> VulkanObject::findNode(int nodeID) {
    // TODO
    // Improve this
    for (int i = 0; i < model->nodes.size(); ++i) {
        for (VulkanNode* node : rootNodes) {
            if (node->nodeID == nodeID) {
                return node;
            }
            for (VulkanNode* child : node->children) {
                if (child->nodeID == nodeID) {
                    return node;
                }
            }
        }
    }
    return std::nullopt;
}

VulkanObject::VulkanObject() {}

void VulkanObject::setPostion(glm::vec3 newPosition) {
    _modelMatrix[3][0] = newPosition.x;
    _modelMatrix[3][1] = newPosition.y;
    _modelMatrix[3][2] = newPosition.z;
    for (VulkanNode* rootNode : rootNodes) {
        rootNode->setLocationMatrix(newPosition);
    }
    for (std::shared_ptr<VulkanObject> child : children) {
        child->setPostion(newPosition);
    }
}
void VulkanObject::setRotation(glm::quat newRotation) {
    _rotation = newRotation;
    updateModelMatrix();
    for (std::shared_ptr<VulkanObject> child : children) {
        child->setRotation(newRotation);
    }
}
void VulkanObject::setScale(glm::vec3 newScale) {
    _scale = newScale;
    updateModelMatrix();
    for (std::shared_ptr<VulkanObject> child : children) {
        child->setScale(newScale);
    }
}

void VulkanObject::x(float newX) {
    _modelMatrix[3][0] = newX;
    updateModelMatrix();
    for (std::shared_ptr<VulkanObject> child : children) {
        child->x(newX);
    }
}
void VulkanObject::y(float newY) {
    _modelMatrix[3][1] = newY;
    updateModelMatrix();
    for (std::shared_ptr<VulkanObject> child : children) {
        child->y(newY);
    }
}
void VulkanObject::z(float newZ) {
    _modelMatrix[3][2] = newZ;
    updateModelMatrix();
    for (std::shared_ptr<VulkanObject> child : children) {
        child->z(newZ);
    }
}

void VulkanObject::updateModelMatrix() {
    _modelMatrix = glm::translate(glm::mat4(1.0f), position()) * glm::toMat4(rotation()) * glm::scale(scale());
    for (VulkanNode* rootNode : rootNodes) {
        rootNode->setLocationMatrix(_modelMatrix);
    }
}

void VulkanObject::keyboardUpdate(GLFWwindow* window, float frameTime) {
    glm::vec3 rotate{0};
    float speedUp = 1;
    if (glfwGetKey(window, keys.yawRight) == GLFW_PRESS)
        rotate.y += 1.f;
    if (glfwGetKey(window, keys.yawLeft) == GLFW_PRESS)
        rotate.y -= 1.f;
    if (glfwGetKey(window, keys.pitchUp) == GLFW_PRESS)
        rotate.x -= 1.f;
    if (glfwGetKey(window, keys.pitchDown) == GLFW_PRESS)
        rotate.x += 1.f;
    if (glfwGetKey(window, keys.rollLeft) == GLFW_PRESS)
        rotate.z -= 1.f;
    if (glfwGetKey(window, keys.rollRight) == GLFW_PRESS)
        rotate.z += 1.f;
    if (glfwGetKey(window, keys.speedUp) == GLFW_PRESS)
        speedUp = 2;
    if (glfwGetKey(window, keys.slowDown) == GLFW_PRESS)
        speedUp = 0.5;

    if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon()) {
        setRotation(rotation() * glm::quat(speedUp * lookSpeed * frameTime * glm::normalize(rotate)));
    }

    const glm::vec3 forwardDir = rotation() * forwardVector;
    const glm::vec3 rightDir = rotation() * rightVector;
    const glm::vec3 upDir = rotation() * upVector;

    glm::vec3 moveDir{0.f};
    speedUp = 1;
    if (glfwGetKey(window, keys.moveForward) == GLFW_PRESS)
        moveDir -= forwardDir;
    if (glfwGetKey(window, keys.moveBackward) == GLFW_PRESS)
        moveDir += forwardDir;
    if (glfwGetKey(window, keys.moveRight) == GLFW_PRESS)
        moveDir -= rightDir;
    if (glfwGetKey(window, keys.moveLeft) == GLFW_PRESS)
        moveDir += rightDir;
    if (glfwGetKey(window, keys.moveUp) == GLFW_PRESS)
        moveDir += upDir;
    if (glfwGetKey(window, keys.moveDown) == GLFW_PRESS)
        moveDir -= upDir;
    if (glfwGetKey(window, keys.speedUp) == GLFW_PRESS)
        speedUp = 2.5;
    if (glfwGetKey(window, keys.slowDown) == GLFW_PRESS)
        speedUp = 0.4;

    if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) {
        setPostion(position() + (speedUp * moveSpeed * frameTime * glm::normalize(moveDir)));
    }

    if (glfwGetKey(window, GLFW_KEY_SPACE))
        std::cout << "Postion: " << glm::to_string(position()) << std::endl << "Rotation: " << glm::to_string(rotation()) << std::endl;
}
