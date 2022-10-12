#include "vulkan_object.hpp"
#include "rapidjson_model.hpp"
#include "vulkan_renderer.hpp"
#include <chrono>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/transform.hpp>
#include <iostream>

VulkanObject::VulkanObject(RapidJSON_Model* model) : model{model} {
    for (RapidJSON_Model::Scene scene : model->scenes) {
        for (int nodeID : scene.nodes) {
            RapidJSON_Model::Node node = model->nodes[nodeID];
            if (node.mesh.has_value()) {
                RapidJSON_Model::Mesh mesh = model->meshes[node.mesh.value()];
                for (RapidJSON_Model::Mesh::Primitive primitive :
                     mesh.primitives) {
                    if (primitive.attributes->position.has_value()) {
                        for (uint32_t i = 0;
                             i < model
                                     ->accessors[primitive.attributes->position
                                                     .value()]
                                     .count;
                             ++i) {
                        }
                    }
                }
            }
        }
    }
}

void VulkanObject::setPostion(glm::vec3 newPosition) {
    isModelMatrixValid = false;
    position = newPosition;
}
void VulkanObject::setRotation(glm::quat newRotation) {
    isModelMatrixValid = false;
    rotation = newRotation;
}
void VulkanObject::setScale(glm::vec3 newScale) {
    isModelMatrixValid = false;
    scale = newScale;
}

void VulkanObject::x(float newX) {
    isModelMatrixValid = false;
    position.x = newX;
}
void VulkanObject::y(float newY) {
    isModelMatrixValid = false;
    position.y = newY;
}
void VulkanObject::z(float newZ) {
    isModelMatrixValid = false;
    position.z = newZ;
}

// TODO
// Take glfwWindow instead of renderer
void VulkanObject::keyboardUpdate(VulkanRenderer* renderer, float frameTime) {
    glm::vec3 rotate{0};
    float speedUp = 1;
    if (glfwGetKey(renderer->getWindow()->getGLFWwindow(), keys.yawRight) ==
        GLFW_PRESS)
        rotate.y -= 1.f;
    if (glfwGetKey(renderer->getWindow()->getGLFWwindow(), keys.yawLeft) ==
        GLFW_PRESS)
        rotate.y += 1.f;
    if (glfwGetKey(renderer->getWindow()->getGLFWwindow(), keys.pitchUp) ==
        GLFW_PRESS)
        rotate.x += 1.f;
    if (glfwGetKey(renderer->getWindow()->getGLFWwindow(), keys.pitchDown) ==
        GLFW_PRESS)
        rotate.x -= 1.f;
    if (glfwGetKey(renderer->getWindow()->getGLFWwindow(), keys.rollLeft) ==
        GLFW_PRESS)
        rotate.z += 1.f;
    if (glfwGetKey(renderer->getWindow()->getGLFWwindow(), keys.rollRight) ==
        GLFW_PRESS)
        rotate.z -= 1.f;
    if (glfwGetKey(renderer->getWindow()->getGLFWwindow(), keys.speedUp) ==
        GLFW_PRESS)
        speedUp = 2;

    if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon()) {
        setRotation(getRotation() * glm::quat(speedUp * lookSpeed * frameTime *
                                              glm::normalize(rotate)));
    }

    const glm::vec3 forwardDir = rotation * glm::vec3(0.0f, 0.0f, -1.0f);
    const glm::vec3 rightDir = rotation * glm::vec3(-1.0f, 0.0, 0.0f);
    const glm::vec3 upDir = rotation * glm::vec3(0.f, -1.0f, 0.f);

    glm::vec3 moveDir{0.f};
    speedUp = 1;
    if (glfwGetKey(renderer->getWindow()->getGLFWwindow(), keys.moveForward) ==
        GLFW_PRESS)
        moveDir += forwardDir;
    if (glfwGetKey(renderer->getWindow()->getGLFWwindow(), keys.moveBackward) ==
        GLFW_PRESS)
        moveDir -= forwardDir;
    if (glfwGetKey(renderer->getWindow()->getGLFWwindow(), keys.moveRight) ==
        GLFW_PRESS)
        moveDir += rightDir;
    if (glfwGetKey(renderer->getWindow()->getGLFWwindow(), keys.moveLeft) ==
        GLFW_PRESS)
        moveDir -= rightDir;
    if (glfwGetKey(renderer->getWindow()->getGLFWwindow(), keys.moveUp) ==
        GLFW_PRESS)
        moveDir += upDir;
    if (glfwGetKey(renderer->getWindow()->getGLFWwindow(), keys.moveDown) ==
        GLFW_PRESS)
        moveDir -= upDir;
    if (glfwGetKey(renderer->getWindow()->getGLFWwindow(), keys.speedUp) ==
        GLFW_PRESS)
        speedUp = 5;

    if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) {
        setPostion(getPosition() +
                   (speedUp * moveSpeed * frameTime * glm::normalize(moveDir)));
    }

    if (glfwGetKey(renderer->getWindow()->getGLFWwindow(), GLFW_KEY_SPACE))
        std::cout << "Postion: " << glm::to_string(getPosition()) << std::endl
                  << "Rotation: " << glm::to_string(getRotation()) << std::endl;
}
