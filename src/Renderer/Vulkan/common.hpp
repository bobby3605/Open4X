#ifndef NEWCOMMON_H_
#define NEWCOMMON_H_

#include <cstdint>
#include <fstream>
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
            throw std::runtime_error((str) + std::to_string((f)));                                                                         \
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
    uint32_t extraObjectCount = 10000;
    uint32_t randLimit = 100;
    bool showFPS = true;
    bool pauseOnMinimization = false;
};

std::string get_file_extension(std::string file_path);

std::string get_filename(std::string file_path);

std::string get_filename_no_ext(std::string file_path);

std::vector<char> read_file(const std::string& filename);

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

#endif // NEWCOMMON_H_
