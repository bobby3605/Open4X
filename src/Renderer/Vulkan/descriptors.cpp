#include "descriptors.hpp"
#include "common.hpp"
#include "device.hpp"
#include <stdexcept>
#include <vulkan/vulkan_core.h>

DescriptorLayout::~DescriptorLayout() {
    for (auto& set_layout_pair : _set_layouts) {
        // FIXME:
        // Only destroy if it has been created
        vkDestroyDescriptorSetLayout(Device::device->vk_device(), set_layout_pair.second.layout, nullptr);
    }
    _descriptor_buffer->free(_descriptor_buffer_allocation);
}

void DescriptorLayout::add_binding(uint32_t set, uint32_t binding, VkDescriptorType type, VkShaderStageFlags stage, std::string buffer_name,
                                   VkMemoryPropertyFlags mem_props) {
    BindingLayout layout;
    layout.binding.binding = binding;
    // TODO:
    // support count
    layout.binding.descriptorCount = 1;
    layout.binding.descriptorType = type;
    layout.binding.stageFlags = stage;
    // TODO:
    // support immutable samplers
    layout.binding.pImmutableSamplers = VK_NULL_HANDLE;
    layout.buffer_name = buffer_name;
    layout.mem_props = mem_props;
    // Create the set mapping if it doesn't exist
    // Add the binding to the set layout
    _set_layouts.try_emplace(set).first->second.bindings.insert(std::pair{binding, layout});
}

void DescriptorLayout::create_layouts() {
    for (auto& set_layout_pair : _set_layouts) {
        SetLayout& set_layout = set_layout_pair.second;
        // Copy bindings info into contiguous memory for VkDescriptorSetLayoutCreateInfo
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.reserve(set_layout.bindings.size());
        for (auto& binding_layout : set_layout.bindings) {
            bindings.push_back(binding_layout.second.binding);
        }
        VkDescriptorSetLayoutCreateInfo info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        // TODO
        // Support more flags
        info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
        info.bindingCount = bindings.size();
        info.pBindings = bindings.data();
        // Create layout
        vkCreateDescriptorSetLayout(Device::device->vk_device(), &info, nullptr, &set_layout.layout);
        // Get size of layout and align it
        vkGetDescriptorSetLayoutSizeEXT_(Device::device->vk_device(), set_layout.layout, &set_layout.layout_size);
        set_layout.layout_size =
            align(set_layout.layout_size, Device::device->descriptor_buffer_properties().descriptorBufferOffsetAlignment);
        _total_layout_size += set_layout.layout_size;
        // Get size of each binding in the layout
        for (auto& binding_layout : set_layout.bindings) {
            vkGetDescriptorSetLayoutBindingOffsetEXT_(Device::device->vk_device(), set_layout.layout, binding_layout.second.binding.binding,
                                                      &binding_layout.second.offset);
        }
    }
    if (_total_layout_size != 0) {
        _descriptor_buffer->alloc(_total_layout_size, _descriptor_buffer_allocation, _descriptor_buffer_offset);
    }
}

std::vector<VkDescriptorSetLayout> DescriptorLayout::vk_set_layouts() const {
    std::vector<VkDescriptorSetLayout> output;
    output.reserve(_set_layouts.size());
    for (auto const& set_layout : _set_layouts) {
        output.push_back(set_layout.second.layout);
    }
    return output;
}

size_t inline descriptor_size_switch(VkDescriptorType type) {
    switch (type) {
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        return Device::device->descriptor_buffer_properties().uniformBufferDescriptorSize;
    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        return Device::device->descriptor_buffer_properties().storageBufferDescriptorSize;
    default:
        throw std::runtime_error("unsupported descriptor type: " + std::to_string(type));
    }
}

uint32_t DescriptorLayout::set_offset(uint32_t set) const {
    // map is sorted, so this will get the correct offset
    uint32_t offset = 0;
    for (auto& set_layout : _set_layouts) {
        if (set_layout.first == set) {
            return offset;
        }
        offset += set_layout.second.layout_size;
    }
    throw std::runtime_error("unknown set num in set_offset: " + std::to_string(set));
}

void DescriptorLayout::set_descriptor_buffer(uint32_t set, uint32_t binding, VkDescriptorDataEXT const& descriptor_data) const {
    SetLayout const& set_layout = _set_layouts.at(set);
    BindingLayout const& binding_layout = set_layout.bindings.at(binding);
    VkDescriptorGetInfoEXT info{VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT};
    info.type = binding_layout.binding.descriptorType;
    info.data = descriptor_data;
    size_t size = descriptor_size_switch(info.type);
    // TODO
    // Support arrays of descriptors by adding array_element * descriptor_size
    char* descriptor_location = _descriptor_buffer->data() + _descriptor_buffer_offset + set_offset(set) + binding_layout.offset;
    vkGetDescriptorEXT_(Device::device->vk_device(), &info, size, descriptor_location);
}
