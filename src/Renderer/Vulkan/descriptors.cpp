#include "descriptors.hpp"
#include "common.hpp"
#include "device.hpp"
#include <cstdint>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

DescriptorLayout::~DescriptorLayout() {
    for (auto& set_layout_pair : _set_layouts) {
        if (set_layout_pair.second.layout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(Device::device->vk_device(), set_layout_pair.second.layout, nullptr);
            if (Device::device->use_descriptor_buffers()) {
                _descriptor_buffer->free(set_layout_pair.second.allocation);
            }
        }
    }
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
    // DescriptorLayout is per pipeline,
    // so if shaders in a pipeline have multiple slots,
    // the descriptor created will be the last one to call add_binding to that slot
    _set_layouts.try_emplace(set).first->second.bindings.emplace(binding, layout);
}

void DescriptorLayout::create_layouts() {
    // NOTE:
    // _set_layouts is a map,
    // so it's sorted,
    // so this will run in set order
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
        if (Device::device->use_descriptor_buffers()) {
            vkGetDescriptorSetLayoutSizeEXT_(Device::device->vk_device(), set_layout.layout, &set_layout.allocation.size);
            // Align it
            set_layout.allocation.size =
                align(set_layout.allocation.size, Device::device->descriptor_buffer_properties().descriptorBufferOffsetAlignment);
            // Allocate size and get the offset for the set
            set_layout.allocation = _descriptor_buffer->alloc(
                set_layout.allocation.size, Device::device->descriptor_buffer_properties().descriptorBufferOffsetAlignment);
            // Get size of each binding in the layout
            for (auto& binding_layout : set_layout.bindings) {
                vkGetDescriptorSetLayoutBindingOffsetEXT_(Device::device->vk_device(), set_layout.layout,
                                                          binding_layout.second.binding.binding, &binding_layout.second.allocation.offset);
            }
        }
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
    return _set_layouts.at(set).allocation.offset;
    throw std::runtime_error("unknown set num in set_offset: " + std::to_string(set));
}

std::vector<VkDeviceSize> DescriptorLayout::set_offsets() const {
    std::vector<VkDeviceSize> set_offsets;
    set_offsets.reserve(_set_layouts.size());
    for (auto const& set_layout : _set_layouts) {
        set_offsets.push_back(set_layout.second.allocation.offset);
    }
    return set_offsets;
}

// FIXME:
// Move this into Buffer::
void DescriptorLayout::set_descriptor_buffer(uint32_t set, uint32_t binding, VkDescriptorDataEXT const& descriptor_data) const {
    SetLayout const& set_layout = _set_layouts.at(set);
    BindingLayout const& binding_layout = set_layout.bindings.at(binding);
    VkDescriptorGetInfoEXT info{VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT};
    info.type = binding_layout.binding.descriptorType;
    info.data = descriptor_data;
    size_t size = descriptor_size_switch(info.type);
    char descriptor[size];
    vkGetDescriptorEXT_(Device::device->vk_device(), &info, size, descriptor);
    SubAllocation descriptor_binding_allocation{};
    descriptor_binding_allocation.size = size;
    // TODO
    // Support arrays of descriptors by adding array_element * descriptor_size
    descriptor_binding_allocation.offset = set_layout.allocation.offset + binding_layout.allocation.offset;
    _descriptor_buffer->write(descriptor_binding_allocation, descriptor, size);
}
