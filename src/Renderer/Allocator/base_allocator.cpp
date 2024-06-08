#include "base_allocator.hpp"

GPUAllocator::GPUAllocator() { gpu_allocator = this; }
CPUAllocator::CPUAllocator() { cpu_allocator = this; }
