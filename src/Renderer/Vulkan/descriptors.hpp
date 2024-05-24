#ifndef DESCRIPTORS_H_
#define DESCRIPTORS_H_
#include "buffer.hpp"
#include <map>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

class DescriptorLayout {
  private:
    struct BindingLayout {
        VkDescriptorSetLayoutBinding binding;
        VkDeviceSize offset;
        std::string buffer_name;
        VkMemoryPropertyFlags mem_props;
    };

    struct SetLayout {
        std::map<uint32_t, BindingLayout> bindings;
        VkDescriptorSetLayout layout;
        VkDeviceSize layout_size;
    };

  public:
    DescriptorLayout(Buffer* descriptor_buffer) : _descriptor_buffer{descriptor_buffer} {}
    ~DescriptorLayout();
    void add_binding(uint32_t set, uint32_t binding, VkDescriptorType type, VkShaderStageFlags stage, std::string buffer_name,
                     VkMemoryPropertyFlags mem_props);
    void create_layouts();
    uint32_t set_offset(uint32_t set) const;
    void set_descriptor_buffer(uint32_t set, uint32_t binding, VkDescriptorDataEXT const& descriptor_data) const;
    std::vector<VkDescriptorSetLayout> vk_set_layouts() const;
    std::map<uint32_t, SetLayout> const& set_layouts() const { return _set_layouts; };

  private:
    std::map<uint32_t, SetLayout> _set_layouts;
    VkDeviceSize _total_layout_size = 0;
    Buffer* _descriptor_buffer;
    VmaVirtualAllocation _descriptor_buffer_allocation = VK_NULL_HANDLE;
    uint64_t _descriptor_buffer_offset = 0;
};

#endif // DESCRIPTORS_H_
