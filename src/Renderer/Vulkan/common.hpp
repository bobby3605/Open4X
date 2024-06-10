#ifndef NEWCOMMON_H_
#define NEWCOMMON_H_

#include <cstdint>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#ifdef NDEBUG
#define check_result(f, str) f
#else
#define check_result(f, str)                                                                                                               \
    {                                                                                                                                      \
        if ((f) != VK_SUCCESS) {                                                                                                           \
            throw std::runtime_error((str) + std::string(" VkResult: ") + std::to_string((f)));                                            \
        }                                                                                                                                  \
    }
#endif

// FIXME:
// Replace with volk
#define def_vk_ext_hpp(f_name) extern PFN_##f_name f_name##_;
#define def_vk_ext_cpp(f_name) PFN_##f_name f_name##_;

def_vk_ext_hpp(vkGetDescriptorSetLayoutSizeEXT);
def_vk_ext_hpp(vkGetDescriptorSetLayoutBindingOffsetEXT);
def_vk_ext_hpp(vkGetDescriptorEXT);
def_vk_ext_hpp(vkCmdBindDescriptorBuffersEXT);
def_vk_ext_hpp(vkCmdSetDescriptorBufferOffsets2EXT);

#define load_instance_addr(f_name, instance) f_name##_ = reinterpret_cast<PFN_##f_name>(vkGetInstanceProcAddr(instance, #f_name))
#define load_device_addr(f_name, device) f_name##_ = reinterpret_cast<PFN_##f_name>(vkGetDeviceProcAddr(device, #f_name))

struct NewSettings {
    uint32_t extra_object_count = 10000;
    uint32_t rand_limit = 100;
    bool show_fps = true;
    bool pause_on_minimization = false;
    uint32_t object_refresh_threads = 2;
};

inline NewSettings* new_settings;

std::string get_file_extension(std::string file_path);

std::string get_filename(std::string file_path);

std::string get_filename_no_ext(std::string file_path);

// NOTE:
// align to a power of 2
inline std::size_t align(std::size_t offset, std::size_t alignment) { return (offset + (alignment - 1)) & -alignment; }

const std::unordered_map<VkBufferUsageFlags, VkDescriptorType> usage_to_types = {
    {VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER},
    {VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER},
    {VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER},
    {VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER},
    {VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC},
    {VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC},
};

VkDescriptorType usage_to_type(VkBufferUsageFlags usage_flags);
VkBufferUsageFlags type_to_usage(VkDescriptorType type);

class MMIO {
    int _fd;
    void* _mapping;
    size_t _size;

  public:
    MMIO(std::filesystem::path const& file_path, int oflag, size_t size = 0);
    void* mapping() { return _mapping; }
    const size_t& size() const { return _size; }
    ~MMIO();
};

template <typename T> void read_file(std::filesystem::path const& file_path, std::vector<T>& buffer) {
    MMIO mmio = MMIO(file_path, O_RDONLY);
    buffer.resize(mmio.size() / sizeof(T));
    memcpy(buffer.data(), mmio.mapping(), mmio.size());
}

template <typename T> void write_file(std::filesystem::path const& file_path, std::vector<T>& buffer) {
    MMIO mmio = MMIO(file_path, O_RDWR | O_CREAT | O_TRUNC, buffer.size() * sizeof(T));
    memcpy(mmio.mapping(), buffer.data(), buffer.size() * sizeof(T));
}

#endif // NEWCOMMON_H_
