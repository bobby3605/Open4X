#include "open4x.hpp"
#include <iostream>

using namespace std;

int main(int argc, char* argv[]) {

    try {
        std::cout << "main" << std::endl;
        Open4X game;
        std::cout << "created" << std::endl;
        game.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
