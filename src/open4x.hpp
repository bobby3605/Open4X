#ifndef OPEN4X_H_
#define OPEN4X_H_

#include "Renderer/Vulkan/model_manager.hpp"
#include "Renderer/Vulkan/object_manager.hpp"
#include "Renderer/Vulkan/renderer.hpp"

struct ShaderGlobals {
    glm::mat4 proj_view;
    glm::vec3 cam_pos;
    uint pad;
};

class Open4X {
  public:
    Open4X();
    ~Open4X();
    void run();

  private:
    void load_settings();

    // New Renderer
    ModelManager* _model_manager;
    ObjectManager* _object_manager;
    Renderer* renderer;
    std::chrono::time_point<std::chrono::system_clock> creation_time;
};

#endif // OPEN4X_H_
