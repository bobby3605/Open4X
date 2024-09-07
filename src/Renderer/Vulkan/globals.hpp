#ifndef GLOBALS_H_
#define GLOBALS_H_

#include "descriptor_manager.hpp"
#include "device.hpp"
#include "memory_manager.hpp"

struct Globals {
    Device* device;
    MemoryManager* memory_manager;
    DescriptorManager* descriptor_manager;
};

static Globals globals{};

#endif // GLOBALS_H_
