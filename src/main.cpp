#include "glTF/GLB.hpp"
#include "glTF/GLTF.hpp"
#include "open4x.hpp"
#include <iostream>

using namespace std;

int main(int argc, char *argv[]) {

  // Open4X game;
  try {
    gltf::GLTF("assets/glTF/basic_triangle.md");
    //  game.run();
    // GLB("assets/glTF/Box.glb");
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
