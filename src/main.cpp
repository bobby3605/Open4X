#include "glTF/GLTF.hpp"
#include "open4x.hpp"
#include <iostream>

using namespace std;

int main(int argc, char *argv[]) {

  // Open4X game;
  try {
    GLTF("assets/glTF/simple_animation.gltf");
    // game.run();
    GLTF("assets/glTF/Box.glb");
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
