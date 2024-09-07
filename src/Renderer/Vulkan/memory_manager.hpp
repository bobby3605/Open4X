#ifndef BUFFERS_H_
#define BUFFERS_H_

#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>

class MemoryManager {
  public:
    MemoryManager();
    static MemoryManager* memory_manager;

    std::unordered_map<std::string, std::vector<VkDescriptorImageInfo>> global_image_infos;
};

#endif // MemoryManager_H_
