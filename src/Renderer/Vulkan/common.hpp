#ifndef COMMON_H_
#define COMMON_H_

#include <cstdint>
#include <stdexcept>

#ifdef NDEBUG
#define check_result(f, str)
#else
#define check_result(f, str)                                                                                                               \
    {                                                                                                                                      \
        if ((f) != VK_SUCCESS) {                                                                                                           \
            throw std::runtime_error((str) + std::to_string((f)));                                                                         \
        }                                                                                                                                  \
    }
#endif

struct NewSettings {
    uint32_t extraObjectCount = 10000;
    uint32_t randLimit = 100;
    bool showFPS = true;
    bool pauseOnMinimization = false;
};

#endif // COMMON_H_
