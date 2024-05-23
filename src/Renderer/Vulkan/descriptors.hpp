#ifndef DESCRIPTORS_H_
#define DESCRIPTORS_H_
#include <map>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

class DescriptorLayout {
  public:
    void add_binding(uint32_t set, uint32_t binding, VkDescriptorType type, VkShaderStageFlags stage, std::string buffer_name,
                     VkMemoryPropertyFlags mem_props);
    void create_layouts();
    uint32_t set_offset(uint32_t set);
    void get_descriptor(uint32_t set, uint32_t binding, VkDescriptorDataEXT const& descriptor_data, void* base_address);
    std::vector<VkDescriptorSetLayout> get_set_layouts() const;

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

    std::map<uint32_t, SetLayout> set_layouts;
};

#endif // DESCRIPTORS_H_
