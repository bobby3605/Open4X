#ifndef DESCRIPTORS_H_
#define DESCRIPTORS_H_
#include "../Allocator/sub_allocator.hpp"
#include <map>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

class DescriptorLayout {
  private:
    struct BindingLayout {
        VkDescriptorSetLayoutBinding binding;
        std::string buffer_name;
        VkMemoryPropertyFlags mem_props;
        SubAllocation allocation;
    };

    struct SetLayout {
        std::map<uint32_t, BindingLayout> bindings;
        VkDescriptorSetLayout layout = VK_NULL_HANDLE;
        SubAllocation allocation;
        VkDescriptorSet set;
    };

  public:
    DescriptorLayout();
    DescriptorLayout(LinearAllocator<GPUAllocator>* descriptor_buffer);
    ~DescriptorLayout();
    void add_binding(uint32_t set, uint32_t binding, VkDescriptorType type, VkShaderStageFlags stage, std::string buffer_name,
                     VkMemoryPropertyFlags mem_props);
    void create_layouts();
    uint32_t set_offset(uint32_t set) const;
    std::vector<VkDeviceSize> set_offsets() const;
    void set_descriptor_buffer(uint32_t set, uint32_t binding, VkDescriptorDataEXT const& descriptor_data) const;
    std::vector<VkDescriptorSetLayout> vk_set_layouts() const;
    std::vector<VkDescriptorSet> vk_sets() const;
    std::map<uint32_t, SetLayout> const& set_layouts() const { return _set_layouts; }

  private:
    std::map<uint32_t, SetLayout> _set_layouts;
    VkDeviceSize _total_layout_size = 0;
    LinearAllocator<GPUAllocator>* _descriptor_buffer = nullptr;
    VkDescriptorPool _pool = VK_NULL_HANDLE;
    static inline const std::vector<VkDescriptorType> extra_types = {VK_DESCRIPTOR_TYPE_SAMPLER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                                     VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                                                                     VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT};
};

#endif // DESCRIPTORS_H_
