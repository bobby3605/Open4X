#include "vulkan_objects.hpp"
#include "../glTF/base64.hpp"
#include <filesystem>
std::string getFileExtension(std::string filePath) {
    return filePath.substr(filePath.find_last_of("."));
}

VulkanObjects::VulkanObjects() {
    for (const auto& filePath :
         std::filesystem::directory_iterator("assets/glTF/")) {
        if (filePath.exists() && filePath.is_regular_file() &&
            getFileExtension(filePath.path()).compare(".gltf") == 0) {
            loadModel(filePath.path());
        }
    }
}
