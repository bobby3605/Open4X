#include "memory_manager.hpp"

MemoryManager* MemoryManager::memory_manager;

MemoryManager::MemoryManager() { memory_manager = this; }
