#include "common.hpp"
#include <vulkan/vulkan_core.h>

def_vk_ext_cpp(vkGetDescriptorSetLayoutSizeEXT);
def_vk_ext_cpp(vkGetDescriptorSetLayoutBindingOffsetEXT);
def_vk_ext_cpp(vkGetDescriptorEXT);
def_vk_ext_cpp(vkCmdBindDescriptorBuffersEXT);
def_vk_ext_cpp(vkCmdSetDescriptorBufferOffsets2EXT);

std::string get_file_extension(std::string file_path) {
    try {
        return file_path.substr(file_path.find_last_of(".") + 1);
    } catch (std::exception& e) {
        throw std::runtime_error("failed to get file extension of: " + file_path);
    }
}

std::string get_filename(std::string file_path) {
    try {
        auto pos = file_path.find_last_of("/\\") + 1;
        // returns npos if / is not found, would happen if there is no subdirectory
        return file_path.substr(pos == std::string::npos ? 0 : pos);
    } catch (std::exception& e) {
        throw std::runtime_error("failed to get filename of: " + file_path);
    }
}

std::string get_filename_no_ext(std::string file_path) {
    try {
        std::string file_name_with_ext = get_filename(file_path);
        return file_name_with_ext.substr(0, file_name_with_ext.find_last_of("."));
    } catch (std::exception& e) {
        throw std::runtime_error("failed to get file extension of: " + file_path);
    }
}

std::vector<char> read_file(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file: " + filename);
    }

    size_t file_size = (size_t)file.tellg();
    std::vector<char> buffer(file_size);
    file.seekg(0);
    file.read(buffer.data(), file_size);
    file.close();
    return buffer;
}

// TODO
// Find a faster way to extract the valid buffer usage flags
// Flags like indirect need to be removed before the map can work properly
VkDescriptorType usage_to_type(VkBufferUsageFlags usage_flags) {
    for (auto& flag_pair : usage_to_types) {
        if ((usage_flags & flag_pair.first) == flag_pair.first) {
            return flag_pair.second;
        }
    }
    // TODO
    // Print hex string
    throw std::runtime_error("Failed to find descriptor type for usage flags: " + std::to_string(usage_flags));
}

VkBufferUsageFlags type_to_usage(VkDescriptorType type) {
    for (auto& flag_pair : usage_to_types) {
        if (type == flag_pair.second) {
            return flag_pair.first;
        }
    }
    // TODO
    // Print hex string or constant name
    throw std::runtime_error("Failed to find buffer usage for descriptor type: " + std::to_string(type));
}
