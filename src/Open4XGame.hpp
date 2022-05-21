#ifndef OPEN4XGAME_H_
#define OPEN4XGAME_H_
#include "Renderer.hpp"
#include "Vulkan/Open4XVulkan.hpp"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Open4XGame {
public:
  Open4XGame();
  ~Open4XGame();
  void mainLoop();

private:
  Renderer *renderer;
};

#endif // OPEN4XGAME_H_
