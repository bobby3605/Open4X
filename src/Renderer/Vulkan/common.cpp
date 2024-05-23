#include "common.hpp"

// TODO
// Find a faster way to extract the valid buffer usage flags
// Flags like indirect need to be removed before the map can work properly
VkDescriptorType usage_to_type(VkBufferUsageFlags usageFlags) {
    for (auto& flag_pair : usage_to_types) {
        if ((usageFlags & flag_pair.first) == flag_pair.first) {
            return flag_pair.second;
        }
    }
    // TODO
    // Print hex string
    throw std::runtime_error("Failed to find descriptor type for usage flags: " + std::to_string(usageFlags));
}
