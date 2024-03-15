#ifndef COMMON_H_
#define COMMON_H_

#define GLM_ENABLE_EXPERIMENTAL

#include "aabb.hpp"
#include <array>
#include <fstream>
#include <functional>
#include <glm/glm.hpp>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.hpp>

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

inline static const glm::vec3 upVector = glm::vec3(0.f, -1.0f, 0.f);
inline static const glm::vec3 forwardVector = glm::vec3(0.0f, 0.0f, -1.0f);
inline static const glm::vec3 rightVector = glm::vec3(-1.0f, 0.0, 0.0f);

typedef std::function<void(VkCommandBuffer)> RenderOp;

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

static std::string getFilename(std::string filePath) {
    try {
        auto pos = filePath.find_last_of("/\\") + 1;
        // returns npos if / is not found, would happen if there is no subdirectory
        return filePath.substr(pos == std::string::npos ? 0 : pos);
    } catch (std::exception& e) {
        throw std::runtime_error("failed to get filename of: " + filePath);
    }
}

static std::string getFilenameNoExt(std::string filePath) {
    try {
        std::string fileNameWithExt = getFilename(filePath);
        return fileNameWithExt.substr(0, filePath.find_last_of("."));
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

struct Plane {
    glm::vec3 normal;
    float distance;
};

struct Frustum {
    Plane planes[6];
};

#endif // COMMON_H_
