#include "open4x.hpp"
#include <iostream>

using namespace std;

int main(int argc, char* argv[]) {

    try {
        Open4X game;
        game.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
