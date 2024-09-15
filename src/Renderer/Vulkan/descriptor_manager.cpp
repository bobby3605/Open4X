#include "descriptor_manager.hpp"
#include "memory_manager.hpp"
#include <list>
#include <stdexcept>

DescriptorManager* DescriptorManager::descriptor_manager;

DescriptorManager::DescriptorManager(GPUAllocator* gpu_allocator) : _gpu_allocator(gpu_allocator) { descriptor_manager = this; }

void DescriptorManager::register_invalid(std::string descriptor_name) { _invalid_descriptors.insert(descriptor_name); }

void DescriptorManager::update(std::shared_ptr<Pipeline> pipeline) {

    std::vector<VkWriteDescriptorSet> descriptor_writes;
    // FIXME:
    // Support only re-binding needed sets
    // For example, a global set at 0 only needs to be set once per GPUAllocator->realloc
    std::list<VkDescriptorBufferInfo> descriptor_buffer_infos;

    for (auto const& set_layout : pipeline->descriptor_layout().set_layouts()) {
        for (auto const& binding_layout : set_layout.second.bindings) {
            std::string const& descriptor_name = binding_layout.second.descriptor_name;
            descriptor_writes.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = set_layout.second.set,
                .dstBinding = binding_layout.second.binding.binding,
                // TODO:
                // only update needed array elements
                .dstArrayElement = 0,
                .descriptorType = binding_layout.second.binding.descriptorType,
            });
            switch (binding_layout.second.binding.descriptorType) {
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                GPUAllocation* buffer;
                if (gpu_allocator->buffer_exists(descriptor_name)) {
                    // TODO:
                    // Maybe
                    // support recreating buffer with new mem props if needed from the graph
                    // Other option is just to error out if you manually created a buffer with the wrong mem_props
                    buffer = gpu_allocator->get_buffer(descriptor_name);
                } else {
                    // FIXME:
                    // vk_buffer isn't created yet, so this causes a validation error with a VK_NULL_HANDLE buffer whenever it runs
                    /*
                    buffer = gpu_allocator->create_buffer(type_to_usage(binding_layout.second.binding.descriptorType),
                                                          binding_layout.second.mem_props | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                                          descriptor_name);
                                                          */
                    throw std::runtime_error("creating buffers in descriptor update is unsupported: " + descriptor_name);
                }
                if (Device::device->use_descriptor_buffers()) {
                    pipeline->descriptor_layout().set_descriptor_buffer(set_layout.first, binding_layout.first, buffer->descriptor_data());
                } else {
                    descriptor_buffer_infos.push_back({
                        .buffer = buffer->buffer(),
                        .offset = 0,
                        .range = VK_WHOLE_SIZE,
                    });
                    descriptor_writes.back().pBufferInfo = &descriptor_buffer_infos.back();
                    descriptor_writes.back().descriptorCount = 1;
                }
                break;
            case VK_DESCRIPTOR_TYPE_SAMPLER:
            case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                descriptor_writes.back().pImageInfo = MemoryManager::memory_manager->global_image_infos[descriptor_name].data();
                descriptor_writes.back().descriptorCount =
                    (uint32_t)MemoryManager::memory_manager->global_image_infos[descriptor_name].size();
                break;
            default:
                throw std::runtime_error("unsupported descriptor type when updating: " +
                                         std::to_string(binding_layout.second.binding.descriptorType));
                break;
            }
        }
    }
    if (!Device::device->use_descriptor_buffers()) {
        vkUpdateDescriptorSets(Device::device->vk_device(), descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);
    }
}
