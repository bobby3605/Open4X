#ifndef VULKAN_OBJECT_H_
#define VULKAN_OBJECT_H_
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "vulkan_model.hpp"
#include "vulkan_renderer.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class VulkanObject {
  public:
    VulkanObject(VulkanModel* model, VulkanRenderer* renderer);
    VulkanObject(VulkanRenderer* renderer);
    void keyboardUpdate(float frameTime);
    glm::vec3 const getPosition() const { return position; }
    glm::quat const getRotation() const { return rotation; }
    glm::vec3 const getScale() const { return scale; }
    void setPostion(glm::vec3 newPosition);
    void setRotation(glm::quat newRotation);
    void setScale(glm::vec3 newScale);
    void x(float newX);
    void y(float newY);
    void z(float newZ);

  private:
    VulkanModel* model;
    VulkanRenderer* renderer;

    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f, 1.0f, 1.0f};

    bool isModelMatrixValid = false;
    glm::mat4 cachedModelMatrix{};

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
        int speedUp = GLFW_KEY_LEFT_SHIFT;
    };
    KeyMappings keys{};

    float moveSpeed{3.0f};
    float lookSpeed{1.5f};
};

#endif // VULKAN_OBJECT_H_
