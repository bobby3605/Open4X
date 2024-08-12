#ifndef GLOBALS_H_
#define GLOBALS_H_

#include "device.hpp"
#include "memory_manager.hpp"

struct Globals {
    Device* device;
    MemoryManager* memory_manager;
};

static Globals globals{};

#endif // GLOBALS_H_