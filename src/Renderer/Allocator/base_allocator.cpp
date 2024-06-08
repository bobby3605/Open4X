#include "base_allocator.hpp"

CPUAllocator::CPUAllocator() { cpu_allocator = this; }
GPUAllocator::GPUAllocator() { gpu_allocator = this; }
