#include "vulkan_objects.hpp"
#include "../glTF/base64.hpp"
#include "rapidjson_model.hpp"
#include "vulkan_object.hpp"
#include <filesystem>
std::string getFileExtension(std::string filePath) { return filePath.substr(filePath.find_last_of(".")); }

VulkanObjects::VulkanObjects() {
    for (const auto& filePath : std::filesystem::directory_iterator("assets/glTF/")) {
        if (filePath.exists() && filePath.is_regular_file() && getFileExtension(filePath.path()).compare(".gltf") == 0) {
            gltf_models.push_back(RapidJSON_Model(filePath.path()));
            objects.push_back(VulkanObject(&gltf_models.back()));
        }
    }
}
