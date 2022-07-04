#ifndef VULKAN_OBJECT_H_
#define VULKAN_OBJECT_H_
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "vulkan_model.hpp"
#include "vulkan_renderer.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class VulkanObject {
public:
  VulkanObject(VulkanModel *model, VulkanRenderer *renderer);
  VulkanObject(VulkanRenderer *renderer);
  void keyboardUpdate(float frameTime);
  void draw();
  glm::mat4 mat4();
  glm::vec3 position{0.0f, 0.0f, 0.0f};
  glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
  glm::vec3 scale{1.0f, 1.0f, 1.0f};

private:
  VulkanModel *model;
  VulkanRenderer *renderer;
  struct KeyMappings {
    int moveLeft = GLFW_KEY_A;
    int moveRight = GLFW_KEY_D;
    int moveForward = GLFW_KEY_W;
    int moveBackward = GLFW_KEY_S;
    int moveUp = GLFW_KEY_Q;
    int moveDown = GLFW_KEY_E;
    int yawLeft = GLFW_KEY_LEFT;
    int yawRight = GLFW_KEY_RIGHT;
    int pitchUp = GLFW_KEY_UP;
    int pitchDown = GLFW_KEY_DOWN;
    int rollLeft = GLFW_KEY_LEFT_CONTROL;
    int rollRight = GLFW_KEY_RIGHT_CONTROL;
  };
  KeyMappings keys{};

  float moveSpeed{3.0f};
  float lookSpeed{1.5f};
};

#endif // VULKAN_OBJECT_H_
