#ifndef COMMON_H_
#define COMMON_H_

#define GLM_ENABLE_EXPERIMENTAL

#include "aabb.hpp"
#include <array>
#include <fstream>
#include <glm/glm.hpp>
#include <iostream>
#include <stdexcept>
#include <vector>

#ifdef NDEBUG
#define checkResult(f, str)
#else
#define checkResult(f, str)                                                                                                                \
    {                                                                                                                                      \
        if ((f) != VK_SUCCESS) {                                                                                                           \
            throw std::runtime_error((str) + std::to_string((f)));                                                                         \
        }                                                                                                                                  \
    }
#endif

// https://github.com/zeux/niagara/blob/master/src/shaders.h#L38
inline uint32_t getGroupCount(uint32_t threadCount, uint32_t localSize) { return (threadCount + localSize - 1) / localSize; }

struct Settings {
    uint32_t extraObjectCount = 10000;
    uint32_t randLimit = 100;
    bool showFPS = true;
    bool pauseOnMinimization = false;
};

static std::string getFileExtension(std::string filePath) {
    try {
        return filePath.substr(filePath.find_last_of(".") + 1);
    } catch (std::exception& e) {
        throw std::runtime_error("failed to get file extension of: " + filePath);
    }
}

static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file: " + filename);
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

#endif // COMMON_H_
