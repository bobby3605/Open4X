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
    // Load nodes and meshes
    for (RapidJSON_Model::Scene scene : model->scenes) {
        // TODO
        // Might fail on multiple scenes
        for (int nodeID : scene.nodes) {
            RapidJSON_Model::Node node = model->nodes[nodeID];
            nodes.push_back(std::make_shared<VulkanNode>(model, nodeID, meshIDMap));
        }
        // TODO
        // Don't iterate twice over nodes
        // TODO
        // segfaults when a child is not listed in scene.nodes
        for (int nodeID : scene.nodes) {
            if (model->nodes[nodeID].children.size() > 0) {
                for (int childNodeID : model->nodes[nodeID].children) {
                    nodes[childNodeID]->_modelMatrix = nodes[nodeID]->modelMatrix() * nodes[childNodeID]->_modelMatrix;
                }
            }
        }
    }
    // Load animation data
    if (model->animations.size() > 0) {
        for (RapidJSON_Model::Animation animation : model->animations) {
            for (std::shared_ptr<RapidJSON_Model::Animation::Sampler> sampler : animation.samplers) {
                RapidJSON_Model::Accessor inputAccessor = model->accessors[sampler->inputIndex];
                for (int i = 0; i < inputAccessor.count; ++i) {
                    sampler->inputData.push_back(loadAccessor<float>(model, &inputAccessor, i));
                }
                RapidJSON_Model::Accessor outputAccessor = model->accessors[sampler->outputIndex];
                for (int i = 0; i < outputAccessor.count; ++i) {
                    sampler->outputData.push_back(glm::make_mat4(glm::value_ptr(loadAccessor<glm::vec4>(model, &outputAccessor, i))));
                }
            }
        }
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
