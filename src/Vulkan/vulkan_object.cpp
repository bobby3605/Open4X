#include "vulkan_object.hpp"
#include <chrono>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/transform.hpp>
#include <iostream>

VulkanObject::VulkanObject(VulkanModel* model, VulkanRenderer* renderer)
    : model{model}, renderer{renderer} {}

void VulkanObject::draw() {
    PushConstants push{};
    push.model = mat4();
    vkCmdPushConstants(
        renderer->getCurrentCommandBuffer(), renderer->getPipelineLayout(),
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
        sizeof(PushConstants), &push);

    model->draw(renderer);
}

VulkanObject::VulkanObject(VulkanRenderer* renderer)
    : model{nullptr}, renderer{renderer} {}

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

void VulkanObject::keyboardUpdate(float frameTime) {
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

glm::mat4 const VulkanObject::mat4() {
    // TODO
    // clean up this terrible logic
    if (!isModelMatrixValid) {
        glm::mat4 translationMatrix = glm::translate(
            glm::mat4(1.0f), {position.x, position.y, position.z});
        glm::mat4 rotationMatrix = glm::toMat4(rotation);
        glm::mat4 scaleMatrix = glm::scale(scale);
        cachedModelMatrix = translationMatrix * rotationMatrix * scaleMatrix;
        isModelMatrixValid = true;
    }

    // TODO
    // make this better and support multiple channels and samplers and
    // animations

    // Model may not be set
    if (model != nullptr) {
        // For some reason, model->gltf_model->animations.empty() segfaults
        // .size() and .capacity segfault too
        if (!model->animationInputs.empty()) {
            for (gltf::Animation animation : model->gltf_model->animations) {
                // TODO
                // support multiple nodes

                float nowAnim =
                    fmod(
                        std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch())
                            .count(),
                        1000 * model->animationInputs.back()) /
                    1000;
                // Get the time since the past second in seconds with 4
                // significant digits
                // Get the animation time
                glm::quat lerp1 = model->animationOutputs[0];
                glm::quat lerp2 = model->animationOutputs[0];
                float previousTime = 0;
                float nextTime = 0;
                for (int i = 0; i < model->animationInputs.size(); ++i) {
                    if (model->animationInputs[i] <= nowAnim) {
                        previousTime = model->animationInputs[i];
                        lerp1 = model->animationOutputs[i];
                    }
                    if (model->animationInputs[i] >= nowAnim) {
                        nextTime = model->animationInputs[i];
                        lerp2 = model->animationOutputs[i];
                        break;
                    }
                }
                glm::mat4 translationMatrix = glm::translate(
                    glm::mat4(1.0f), {position.x, position.y, position.z});
                float interpolationValue =
                    (nowAnim - previousTime) / (nextTime - previousTime);
                glm::mat4 rotationMatrix = glm::toMat4(
                    rotation * glm::slerp(lerp1, lerp2, interpolationValue));
                glm::mat4 scaleMatrix = glm::scale(scale);
                isModelMatrixValid = true;
                return translationMatrix * rotationMatrix * scaleMatrix;
            }
        }
    }
    return cachedModelMatrix;
}
