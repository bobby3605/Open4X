#include "vulkan_object.hpp"
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

VulkanObject::VulkanObject(VulkanModel* model, VulkanRenderer* renderer)
    : model{model}, renderer{renderer} {}

void VulkanObject::draw() {
    if (model->gltf_model != nullptr) {
        for (gltf::Scene scene : model->gltf_model->scenes) {
            for (int nodeID : scene.nodes) {

                PushConstants push{};
                push.model = mat4(nodeID);
                vkCmdPushConstants(renderer->getCurrentCommandBuffer(),
                                   renderer->getPipelineLayout(),
                                   VK_SHADER_STAGE_VERTEX_BIT |
                                       VK_SHADER_STAGE_FRAGMENT_BIT,
                                   0, sizeof(PushConstants), &push);
                // TODO
                // drawIndirect is running once for each node
                // It should run once per gltf model
                // push constants for model matrix should be converted into a
                // SSBO, which is indexed into by instanceID

                model->drawIndirect(renderer);
            }
        }
    } else {
        PushConstants push{};
        push.model = mat4();
        vkCmdPushConstants(
            renderer->getCurrentCommandBuffer(), renderer->getPipelineLayout(),
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
            sizeof(PushConstants), &push);

        model->draw(renderer);
    }
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

glm::mat4 const VulkanObject::mat4(uint32_t node) {
    // TODO
    // clean up this terrible logic
    if (!isModelMatrixValid || isModelMatrixValid) {
        glm::vec3 modelTranslation(0.0f);
        glm::quat modelRotation(0.0f, 0.0f, 0.0f, 0.0f);
        glm::vec3 modelScale(0.0f);

        if ((model != nullptr) && (model->gltf_model != nullptr)) {
            modelTranslation = model->gltf_model->nodes[node].translation;
            modelRotation = model->gltf_model->nodes[node].rotation;
            modelScale = model->gltf_model->nodes[node].scale;
        }

        glm::mat4 translationMatrix =
            glm::translate(glm::mat4(1.0f), position + modelTranslation);
        glm::mat4 rotationMatrix = glm::toMat4(rotation + modelRotation);
        glm::mat4 scaleMatrix = glm::scale(scale + modelScale);
        cachedModelMatrix = translationMatrix * rotationMatrix * scaleMatrix;
        isModelMatrixValid = true;
    }

    // TODO
    // Make this faster
    // Model may not be set
    if (model != nullptr) {
        if (model->gltf_model != nullptr) {
            if (!model->gltf_model->animations.empty()) {
                for (gltf::Animation animation :
                     model->gltf_model->animations) {
                    glm::vec3 translationAnimation(1.0f);
                    glm::quat rotationAnimation(1.0f, 0.0f, 0.0f, 0.0f);
                    glm::vec3 scaleAnimation(1.0f);
                    for (std::shared_ptr<gltf::Animation::Channel> channel :
                         animation.channels) {
                        if (channel->target->node == node) {
                            std::shared_ptr<gltf::Animation::Sampler> sampler =
                                animation.samplers[channel->sampler];

                            float nowAnim =
                                fmod(std::chrono::duration_cast<
                                         std::chrono::milliseconds>(
                                         std::chrono::system_clock::now()
                                             .time_since_epoch())
                                         .count(),
                                     1000 * sampler->inputData.back()) /
                                1000;
                            // Get the time since the past second in seconds
                            // with 4 significant digits Get the animation time
                            glm::mat4 lerp1 = sampler->outputData.front();
                            glm::mat4 lerp2 = sampler->outputData.front();
                            float previousTime = 0;
                            float nextTime = 0;
                            for (int i = 0; i < sampler->inputData.size();
                                 ++i) {
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
                            float interpolationValue =
                                (nowAnim - previousTime) /
                                (nextTime - previousTime);

                            if (channel->target->path.compare("translation") ==
                                0) {

                                translationAnimation =
                                    glm::make_vec3(glm::value_ptr(lerp1)) +
                                    interpolationValue *
                                        (glm::make_vec3(glm::value_ptr(lerp2)) -
                                         glm::make_vec3(glm::value_ptr(lerp1)));

                                std::cout
                                    << "translationAnimation: "
                                    << glm::to_string(translationAnimation)
                                    << std::endl;

                            } else if (channel->target->path.compare(
                                           "rotation") == 0) {

                                rotationAnimation = glm::slerp(
                                    glm::make_quat(glm::value_ptr(lerp1)),
                                    glm::make_quat(glm::value_ptr(lerp2)),
                                    interpolationValue);

                            } else if (channel->target->path.compare("scale") ==
                                       0) {
                                scaleAnimation =
                                    glm::make_vec3(glm::value_ptr(lerp1)) +
                                    interpolationValue *
                                        (glm::make_vec3(glm::value_ptr(lerp2)) -
                                         glm::make_vec3(glm::value_ptr(lerp1)));

                            } else {
                                std::cout << "Unknown channel type: "
                                          << channel->target->path << std::endl;
                            }
                        }
                    }

                    glm::mat4 translationMatrix = glm::translate(
                        glm::mat4(1.0f), translationAnimation * position);

                    glm::mat4 rotationMatrix =
                        glm::toMat4(rotationAnimation * rotation);

                    glm::mat4 scaleMatrix = glm::scale(scaleAnimation * scale);

                    return translationMatrix * rotationMatrix * scaleMatrix;
                }
            }
        }
    }
    return cachedModelMatrix;
}
