#ifndef NEWCOMMON_H_
#define NEWCOMMON_H_

#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>

#ifdef NDEBUG
#define check_result(f, str)
#else
#define check_result(f, str)                                                                                                               \
    {                                                                                                                                      \
        if ((f) != VK_SUCCESS) {                                                                                                           \
            throw std::runtime_error((str) + std::to_string((f)));                                                                         \
        }                                                                                                                                  \
    }
#endif

struct NewSettings {
    uint32_t extraObjectCount = 10000;
    uint32_t randLimit = 100;
    bool showFPS = true;
    bool pauseOnMinimization = false;
};

static std::string get_file_extension(std::string file_path) {
    try {
        return file_path.substr(file_path.find_last_of(".") + 1);
    } catch (std::exception& e) {
        throw std::runtime_error("failed to get file extension of: " + file_path);
    }
}

static std::string get_filename(std::string file_path) {
    try {
        auto pos = file_path.find_last_of("/\\") + 1;
        // returns npos if / is not found, would happen if there is no subdirectory
        return file_path.substr(pos == std::string::npos ? 0 : pos);
    } catch (std::exception& e) {
        throw std::runtime_error("failed to get filename of: " + file_path);
    }
}

static std::string get_filename_no_ext(std::string file_path) {
    try {
        std::string file_name_with_ext = get_filename(file_path);
        return file_name_with_ext.substr(0, file_path.find_last_of("."));
    } catch (std::exception& e) {
        throw std::runtime_error("failed to get file extension of: " + file_path);
    }
}

static std::vector<char> read_file(const std::string& filename) {
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

static inline std::size_t align(std::size_t x, std::size_t U) { return (sizeof(x) + sizeof(U) - 1) - sizeof(x) % sizeof(U); }

static inline const std::unordered_map<VkBufferUsageFlags, VkDescriptorType> usage_to_types = {
    {VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER},
    {VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER},
    {VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER},
    {VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER},
    {VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC},
    {VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC},
};

VkDescriptorType usage_to_type(VkBufferUsageFlags usageFlags);

#endif // NEWCOMMON_H_
