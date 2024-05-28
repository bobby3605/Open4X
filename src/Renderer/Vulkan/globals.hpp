#ifndef GLOBALS_H_
#define GLOBALS_H_

#include "device.hpp"

struct Globals {
    Device* device;
};

static Globals globals{};

#endif // GLOBALS_H_
