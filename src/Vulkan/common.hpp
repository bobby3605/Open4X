#ifndef COMMON_H_
#define COMMON_H_

#include <stdexcept>

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

#endif // COMMON_H_
