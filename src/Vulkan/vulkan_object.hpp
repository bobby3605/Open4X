#ifndef VULKAN_OBJECT_H_
#define VULKAN_OBJECT_H_
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "vulkan_model.hpp"
#include "vulkan_node.hpp"
#include "vulkan_renderer.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class VulkanObject {
  public:
    VulkanObject(std::shared_ptr<VulkanModel> vulkanModel, std::shared_ptr<SSBOBuffers> ssboBuffers, std::string const& name,
                 bool duplicate = false);
    VulkanObject();
    ~VulkanObject();
    std::string const name() { return _name; }
    void keyboardUpdate(GLFWwindow* window, float frameTime);
    glm::vec3 const position() const { return _position; }
    glm::quat const rotation() const { return _rotation; }
    glm::vec3 const scale() const { return _scale; }
    void setPostion(glm::vec3 newPosition);
    void setRotation(glm::quat newRotation);
    void setScale(glm::vec3 newScale);
    void x(float newX);
    void y(float newY);
    void z(float newZ);
    glm::mat4 const modelMatrix() { return _modelMatrix; }
    void draw();

    void draw(VulkanRenderer* renderer);
    std::shared_ptr<GLTF> model;
    std::vector<VulkanNode*> rootNodes;

    std::optional<VulkanNode*> findNode(int nodeID);
    void updateAnimations(std::shared_ptr<SSBOBuffers> ssboBuffers);
    void uploadModelMatrices(std::shared_ptr<SSBOBuffers> ssboBuffers);

    std::vector<std::shared_ptr<VulkanObject>> children;

    inline static const glm::vec3 upVector = glm::vec3(0.f, -1.0f, 0.f);
    inline static const glm::vec3 forwardVector = glm::vec3(0.0f, 0.0f, -1.0f);
    inline static const glm::vec3 rightVector = glm::vec3(-1.0f, 0.0, 0.0f);

  private:
    char* _name = nullptr;

    std::vector<VulkanNode*>* animatedNodes = nullptr;

    glm::vec3 _position{0.0f, 0.0f, 0.0f};
    glm::quat _rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 _scale{1.0f, 1.0f, 1.0f};

    glm::mat4 _modelMatrix{1.0f};
    void updateModelMatrix();

    struct KeyMappings {
        static const inline int moveLeft = GLFW_KEY_A;
        static const inline int moveRight = GLFW_KEY_D;
        static const inline int moveForward = GLFW_KEY_W;
        static const inline int moveBackward = GLFW_KEY_S;
        static const inline int moveUp = GLFW_KEY_Q;
        static const inline int moveDown = GLFW_KEY_E;
        static const inline int yawLeft = GLFW_KEY_LEFT;
        static const inline int yawRight = GLFW_KEY_RIGHT;
        static const inline int pitchUp = GLFW_KEY_UP;
        static const inline int pitchDown = GLFW_KEY_DOWN;
        static const inline int rollLeft = GLFW_KEY_LEFT_CONTROL;
        static const inline int rollRight = GLFW_KEY_RIGHT_CONTROL;
        static const inline int speedUp = GLFW_KEY_LEFT_SHIFT;
        static const inline int slowDown = GLFW_KEY_LEFT_ALT;
    };
    static constexpr KeyMappings keys{};

    float moveSpeed{6.0f};
    float lookSpeed{2.0f};
};

#endif // VULKAN_OBJECT_H_
