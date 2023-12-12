#include "vulkan_descriptors.hpp"
#include "../../external/stb_image.h"
#include "common.hpp"
#include "vulkan_swapchain.hpp"
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>

VulkanDescriptors::VulkanDescriptor::VulkanDescriptor(VulkanDescriptors* descriptorManager, VkShaderStageFlags stageFlags)
    : descriptorManager{descriptorManager}, _stageFlags{stageFlags} {}

VulkanDescriptors::VulkanDescriptor::~VulkanDescriptor() {
    vkDestroyDescriptorSetLayout(descriptorManager->device->device(), layout, nullptr);
}

// TODO
// Find a faster way to extract the valid buffer usage flags
// Flags like indirect need to be removed before the map can work properly
VkDescriptorType VulkanDescriptors::getType(VkBufferUsageFlags usageFlags) {
    for (auto flagPair : usageToTypes) {
        if ((usageFlags & flagPair.first) == flagPair.first) {
            return flagPair.second;
        }
    }
    // TODO
    // Print hex string
    throw std::runtime_error("Failed to find descriptor type for usage flags: " + std::to_string(usageFlags));
}

void VulkanDescriptors::VulkanDescriptor::addBinding(uint32_t bindingID, std::vector<VkDescriptorImageInfo>& imageInfos, uint32_t setID) {

    VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;

    if (imageInfos[0].imageView != VK_NULL_HANDLE) {
        descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    }

    if (setID == 0) {
        VkDescriptorSetLayoutBinding binding{};
        binding.binding = bindingID;
        binding.descriptorType = descriptorType;
        binding.descriptorCount = imageInfos.size();
        binding.pImmutableSamplers = nullptr;
        binding.stageFlags = _stageFlags;

        bindings.insert({bindingID, binding});
    }

    _imageInfos.insert({{setID, bindingID}, imageInfos.data()});
}

void VulkanDescriptors::VulkanDescriptor::addBinding(uint32_t bindingID, std::shared_ptr<VulkanBuffer> buffer, uint32_t setID) {
    if (setID == 0) {
        VkDescriptorSetLayoutBinding binding{};
        binding.binding = bindingID;
        binding.descriptorType = getType(buffer->usageFlags());
        binding.descriptorCount = 1;
        binding.pImmutableSamplers = nullptr;
        binding.stageFlags = _stageFlags;

        bindings.insert({bindingID, binding});
    }

    bufferInfos.insert({{setID, bindingID}, buffer->bufferInfo()});
}

void VulkanDescriptors::VulkanDescriptor::createLayout() {
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    std::vector<VkDescriptorSetLayoutBinding> _bindings;
    for (auto b : bindings) {
        _bindings.push_back(b.second);
    }
    layoutInfo.pBindings = _bindings.data();

    checkResult(vkCreateDescriptorSetLayout(descriptorManager->device->device(), &layoutInfo, nullptr, &layout),
                "failed to create descriptor set layout");
}

void VulkanDescriptors::VulkanDescriptor::allocateSets(uint32_t count) {
    createLayout();
    for (uint32_t i = 0; i < count; ++i) {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorManager->pool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &layout;

        VkDescriptorSet set;
        checkResult(vkAllocateDescriptorSets(descriptorManager->device->device(), &allocInfo, &set), "failed to allocate descriptor sets");
        sets.push_back(set);
    }
}

void VulkanDescriptors::VulkanDescriptor::update() {
    for (uint32_t setIndex = 0; setIndex < sets.size(); ++setIndex) {
        std::vector<VkWriteDescriptorSet> descriptorWrites(bindings.size());
        for (uint32_t bindingIndex = 0; bindingIndex < bindings.size(); ++bindingIndex) {
            descriptorWrites[bindingIndex].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[bindingIndex].dstSet = sets[setIndex];
            descriptorWrites[bindingIndex].dstBinding = bindings[bindingIndex].binding;
            descriptorWrites[bindingIndex].dstArrayElement = 0;
            descriptorWrites[bindingIndex].descriptorType = bindings[bindingIndex].descriptorType;
            descriptorWrites[bindingIndex].descriptorCount = bindings[bindingIndex].descriptorCount;
            switch (bindings[bindingIndex].descriptorType) {
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                descriptorWrites[bindingIndex].pBufferInfo = &bufferInfos[{setIndex, bindingIndex}];
                break;
            case VK_DESCRIPTOR_TYPE_SAMPLER:
            case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                descriptorWrites[bindingIndex].pImageInfo = _imageInfos[{setIndex, bindingIndex}];
                break;
            default:
                throw std::runtime_error("unknown descriptor type");
            }
        }
        vkUpdateDescriptorSets(descriptorManager->device->device(), bindings.size(), descriptorWrites.data(), 0, nullptr);
    }
}

VulkanDescriptors::VulkanDescriptors(VulkanDevice* deviceRef) : device{deviceRef} { pool = createPool(); }

VulkanDescriptors::VulkanDescriptor* VulkanDescriptors::createDescriptor(std::string name, VkShaderStageFlags stageFlags) {
    VulkanDescriptor* tmp = new VulkanDescriptor(this, stageFlags);
    descriptors.insert({name, tmp});
    return tmp;
}

VkDescriptorPool VulkanDescriptors::createPool() {
    int count = 10 * VulkanSwapChain::MAX_FRAMES_IN_FLIGHT;
    std::vector<VkDescriptorPoolSize> poolSizes{};
    poolSizes.reserve(usageToTypes.size() + extraTypes.size());
    for (auto typePair : usageToTypes) {
        poolSizes.push_back({typePair.second, uint32_t(count)});
    }
    for (auto type : extraTypes) {
        poolSizes.push_back({type, uint32_t(count)});
    }

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = count * poolSizes.size();

    VkDescriptorPool pool;
    checkResult(vkCreateDescriptorPool(device->device(), &poolInfo, nullptr, &pool), "failed to create descriptor pool");
    return pool;
}

VulkanDescriptors::~VulkanDescriptors() {
    for (auto& descriptor : descriptors)
        delete descriptor.second;

    vkDestroyDescriptorPool(device->device(), pool, nullptr);
}
