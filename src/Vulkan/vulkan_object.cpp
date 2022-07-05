#include "vulkan_object.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/transform.hpp>
#include <iostream>

VulkanObject::VulkanObject(VulkanModel *model, VulkanRenderer *renderer) : model{model}, renderer{renderer} {}

void VulkanObject::draw() {
  PushConstants push{};
  // TODO cache mat4
  push.model = mat4();
  vkCmdPushConstants(renderer->getCurrentCommandBuffer(), renderer->getPipelineLayout(),
                     VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &push);

  model->draw(renderer);
}

VulkanObject::VulkanObject(VulkanRenderer *renderer) : renderer{renderer} {}

void VulkanObject::keyboardUpdate(float frameTime) {
  glm::vec3 rotate{0};
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

  if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon()) {
    rotation *= glm::quat(lookSpeed * frameTime * glm::normalize(rotate));
  }

  const glm::vec3 forwardDir = rotation * glm::vec3(0.0f, 0.0f, -1.0f);
  const glm::vec3 rightDir = rotation * glm::vec3(-1.0f, 0.0, 0.0f);
  const glm::vec3 upDir = rotation * glm::vec3(0.f, -1.0f, 0.f);

  glm::vec3 moveDir{0.f};
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

  if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) {
    position += moveSpeed * frameTime * glm::normalize(moveDir);
  }

  if (glfwGetKey(renderer->getWindow()->getGLFWwindow(), GLFW_KEY_SPACE))
    std::cout << "Postion: " << glm::to_string(position) << std::endl
              << "Rotation: " << glm::to_string(rotation) << std::endl;
}

glm::mat4 VulkanObject::mat4() {
  glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), {position.x, position.y, position.z});
  glm::mat4 rotationMatrix = glm::toMat4(rotation);
  glm::mat4 scaleMatrix = glm::scale(scale);
  glm::mat4 modelMatrix = translationMatrix * rotationMatrix * scaleMatrix;

  return modelMatrix;
}
