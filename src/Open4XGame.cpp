#include "Open4XGame.hpp"
#include "Open4XVulkan.hpp"
#include <GLFW/glfw3.h>

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, 1);
  }
}

Open4XGame::Open4XGame() {
  renderer = new Open4XVulkan();
  glfwSetKeyCallback(renderer->window, key_callback);
}

Open4XGame::~Open4XGame() {
}

void Open4XGame::mainLoop() {
  while (!glfwWindowShouldClose(renderer->window)) {
    glfwPollEvents();
    renderer->drawFrame();
  }

  delete renderer;

}
