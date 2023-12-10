#ifndef COMMON_H_
#define COMMON_H_

#include <stdexcept>
#include <glm/glm.hpp>

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
};

struct AABB {
    glm::vec3 max{-MAXFLOAT};
    glm::vec3 min{MAXFLOAT};
    inline void update(glm::vec3 newBounds) {
        max.x = glm::max(max.x, newBounds.x);
        max.y = glm::max(max.y, newBounds.y);
        max.z = glm::max(max.z, newBounds.z);

        min.x = glm::min(min.x, newBounds.x);
        min.y = glm::min(min.y, newBounds.y);
        min.z = glm::min(min.z, newBounds.z);
    }

    inline void update(AABB newBounds) {
        max.x = glm::max(max.x, newBounds.max.x);
        max.y = glm::max(max.y, newBounds.max.y);
        max.z = glm::max(max.z, newBounds.max.z);

        min.x = glm::min(min.x, newBounds.min.x);
        min.y = glm::min(min.y, newBounds.min.y);
        min.z = glm::min(min.z, newBounds.min.z);
    }

    inline void update(glm::vec4 newBounds) {
        update(glm::vec3(newBounds));
    }
};

#endif // COMMON_H_
