#include "vulkan_object.hpp"
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

VulkanObject::VulkanObject(std::shared_ptr<GLTF> model, std::shared_ptr<SSBOBuffers> ssboBuffers, std::string name)
    : model{model}, _name{name} {
    // Load nodes and meshes
    for (GLTF::Scene scene : model->scenes) {
        for (int rootNodeID : scene.nodes) {
            GLTF::Node node = model->nodes[rootNodeID];
            rootNodes.push_back(std::make_shared<VulkanNode>(model, rootNodeID, &meshIDMap, &materialIDMap, ssboBuffers));
        }
        // Load animation data
        for (GLTF::Animation animation : model->animations) {
            for (std::shared_ptr<GLTF::Animation::Channel> channel : animation.channels) {
                std::shared_ptr<GLTF::Animation::Sampler> sampler = animation.samplers[channel->sampler];
                std::optional<std::shared_ptr<VulkanNode>> node = findNode(channel->target->node);
                if (node.has_value()) {
                    animatedNodes.push_back(node.value());
                    node.value()->animationPair = {channel, sampler};
                }
                GLTF::Accessor* inputAccessor = &model->accessors[sampler->inputIndex];
                for (int i = 0; i < inputAccessor->count; ++i) {
                    sampler->inputData.push_back(loadAccessor<float>(model, inputAccessor, i));
                }
                GLTF::Accessor* outputAccessor = &model->accessors[sampler->outputIndex];
                for (int i = 0; i < outputAccessor->count; ++i) {
                    sampler->outputData.push_back(glm::make_mat4(glm::value_ptr(loadAccessor<glm::vec4>(model, outputAccessor, i))));
                }
            }
        }
    }
}

void VulkanObject::updateAnimations() {
    for (std::shared_ptr<VulkanNode> node : animatedNodes) {
        node->updateAnimation();
    }
}

std::optional<std::shared_ptr<VulkanNode>> VulkanObject::findNode(int nodeID) {
    // TODO
    // Improve this
    for (int i = 0; i < model->nodes.size(); ++i) {
        for (std::shared_ptr<VulkanNode> node : rootNodes) {
            if (node->nodeID == nodeID) {
                return node;
            }
            for (std::shared_ptr<VulkanNode> child : node->children) {
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
    _position = newPosition;
    updateModelMatrix();
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
    _position.x = newX;
    updateModelMatrix();
    for (std::shared_ptr<VulkanObject> child : children) {
        child->x(newX);
    }
}
void VulkanObject::y(float newY) {
    _position.y = newY;
    updateModelMatrix();
    for (std::shared_ptr<VulkanObject> child : children) {
        child->y(newY);
    }
}
void VulkanObject::z(float newZ) {
    _position.z = newZ;
    updateModelMatrix();
    for (std::shared_ptr<VulkanObject> child : children) {
        child->z(newZ);
    }
}

void VulkanObject::updateModelMatrix() {
    _modelMatrix = glm::translate(glm::mat4(1.0f), position()) * glm::toMat4(rotation()) * glm::scale(scale());
    for (std::shared_ptr<VulkanNode> rootNode : rootNodes) {
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

    const glm::vec3 forwardDir = rotation() * glm::vec3(0.0f, 0.0f, -1.0f);
    const glm::vec3 rightDir = rotation() * glm::vec3(-1.0f, 0.0, 0.0f);
    const glm::vec3 upDir = rotation() * glm::vec3(0.f, -1.0f, 0.f);

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
