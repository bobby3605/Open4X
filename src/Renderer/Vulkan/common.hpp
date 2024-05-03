#ifndef COMMON_H_
#define COMMON_H_

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

#endif // COMMON_H_
