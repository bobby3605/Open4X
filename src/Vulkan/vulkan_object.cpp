#include "vulkan_object.hpp"
#include "../glTF/AccessorLoader.hpp"
#include "vulkan_buffer.hpp"
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
#include <memory>
#include <vulkan/vulkan_core.h>

VulkanObject::VulkanObject(std::shared_ptr<VulkanModel> model, std::shared_ptr<SSBOBuffers> ssboBuffers, std::string const& name,
                           bool duplicate)
    : model{model} {
    _name = new char[name.size()];
    for (int i = 0; i < name.size(); ++i) {
        _name[i] = name[i];
    }
    // NOTE:
    // Preallocates instance ids from the total instance ids in the model
    firstInstanceID = ssboBuffers->uniqueInstanceID.fetch_add(model->totalInstanceCount(), std::memory_order_relaxed);
    model->addInstance(firstInstanceID, ssboBuffers);
}

VulkanObject::~VulkanObject() {
    if (_name != nullptr) {
        delete _name;
    }
}

VulkanObject::VulkanObject() {}

void VulkanObject::setPostion(glm::vec3 newPosition) {
    _position = newPosition;
    for (std::shared_ptr<VulkanObject> child : children) {
        child->setPostion(newPosition);
    }
}
void VulkanObject::setRotation(glm::quat newRotation) {
    _rotation = newRotation;
    for (std::shared_ptr<VulkanObject> child : children) {
        child->setRotation(newRotation);
    }
}
void VulkanObject::setScale(glm::vec3 newScale) {
    _scale = newScale;
    for (std::shared_ptr<VulkanObject> child : children) {
        child->setScale(newScale);
    }
}

void VulkanObject::x(float newX) {
    _position.x = newX;
        if (model->model->fileName() == "GearboxAssy.glb") {
        }
    for (std::shared_ptr<VulkanObject> child : children) {
        child->x(newX);
    }
}
void VulkanObject::y(float newY) {
    _position.y = newY;
    for (std::shared_ptr<VulkanObject> child : children) {
        child->y(newY);
    }
}
void VulkanObject::z(float newZ) {
    _position.z = newZ;
    for (std::shared_ptr<VulkanObject> child : children) {
        child->z(newZ);
    }
}

void VulkanObject::updateModelMatrix(std::shared_ptr<SSBOBuffers> ssboBuffers) {
    // Calculate offset to normalize position into world space
    glm::vec3 positionOffset{0, 0, 0};
    if (model != nullptr && model->centerpoint.has_value()) {
        positionOffset = model->centerpoint.value() * scale();
    }
    glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), position() - positionOffset) * glm::toMat4(rotation()) * glm::scale(scale());
    model->uploadModelMatrix(firstInstanceID, modelMatrix, ssboBuffers);
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
