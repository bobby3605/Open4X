#include <iostream>
#include "open4x.hpp"

using namespace std;

int main(int argc, char *argv[]) {

  Open4X game;
  try {
    game.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

