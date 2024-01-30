#include "vulkan_descriptors.hpp"
#include "common.hpp"
#include "stb/stb_image.h"
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
    for (auto layoutPair : layouts) {
        vkDestroyDescriptorSetLayout(descriptorManager->device->device(), layoutPair.second, nullptr);
    }
}

/*
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
*/

void VulkanDescriptors::VulkanDescriptor::addBinding(uint32_t bindingID, std::vector<VkDescriptorImageInfo>& imageInfos, uint32_t setID) {

    VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;

    if (imageInfos[0].imageView != VK_NULL_HANDLE) {
        descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    }

    VkDescriptorSetLayoutBinding binding{};
    binding.binding = bindingID;
    binding.descriptorType = descriptorType;
    binding.descriptorCount = imageInfos.size();
    binding.pImmutableSamplers = nullptr;
    binding.stageFlags = _stageFlags;

    bindings.insert({{setID, bindingID}, binding});

    _imageInfos.insert({{setID, bindingID}, imageInfos.data()});
    uniqueSetIDs.insert(setID);
}

void VulkanDescriptors::VulkanDescriptor::addBinding(uint32_t setID, uint32_t bindingID, VkDescriptorType type) {
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = bindingID;
    binding.descriptorType = type;
    binding.descriptorCount = 1;
    binding.pImmutableSamplers = nullptr;
    binding.stageFlags = _stageFlags;

    bindings.insert({{setID, bindingID}, binding});

    uniqueSetIDs.insert(setID);
}

void VulkanDescriptors::VulkanDescriptor::setBuffer(uint32_t setID, uint32_t bindingID, VkDescriptorBufferInfo bufferInfo) {
    bufferInfos.insert({{setID, bindingID}, bufferInfo});
}

void VulkanDescriptors::VulkanDescriptor::createLayout(uint32_t setID) {
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    std::vector<VkDescriptorSetLayoutBinding> _bindings;
    for (auto b : bindings) {
        _bindings.push_back(b.second);
    }
    layoutInfo.pBindings = _bindings.data();

    VkDescriptorSetLayout layout;
    checkResult(vkCreateDescriptorSetLayout(descriptorManager->device->device(), &layoutInfo, nullptr, &layout),
                "failed to create descriptor set layout");
    layouts.insert({setID, layout});
}

std::vector<VkDescriptorSetLayout> VulkanDescriptors::VulkanDescriptor::getLayouts() {
    std::vector<VkDescriptorSetLayout> flatLayouts(layouts.size());
    for (auto layout : layouts) {
        flatLayouts.push_back(layout.second);
    }
    return flatLayouts;
}

void VulkanDescriptors::VulkanDescriptor::allocateSets() {
    for (auto uniqueSetID : uniqueSetIDs) {
        createLayout(uniqueSetID);
    }
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorManager->pool;
    allocInfo.descriptorSetCount = uniqueSetIDs.size();
    std::vector<VkDescriptorSetLayout> tmpLayouts(uniqueSetIDs.size());
    for (auto layout : layouts) {
        tmpLayouts.push_back(layout.second);
    }
    allocInfo.pSetLayouts = tmpLayouts.data();

    std::vector<VkDescriptorSet> tmpSets(uniqueSetIDs.size());
    checkResult(vkAllocateDescriptorSets(descriptorManager->device->device(), &allocInfo, tmpSets.data()),
                "failed to allocate descriptor sets");
    uint32_t i = 0;
    for (auto layout : layouts) {
        sets[layout.first] = tmpSets[i];
        ++i;
    }
}

void VulkanDescriptors::VulkanDescriptor::update() {
    for (auto it : sets) {
        std::vector<VkWriteDescriptorSet> descriptorWrites(bindings.size());
        for (uint32_t bindingIndex = 0; bindingIndex < bindings.size(); ++bindingIndex) {
            descriptorWrites[bindingIndex].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[bindingIndex].dstSet = it.second;
            descriptorWrites[bindingIndex].dstBinding = bindings[{it.first, bindingIndex}].binding;
            descriptorWrites[bindingIndex].dstArrayElement = 0;
            descriptorWrites[bindingIndex].descriptorType = bindings[{it.first, bindingIndex}].descriptorType;
            descriptorWrites[bindingIndex].descriptorCount = bindings[{it.first, bindingIndex}].descriptorCount;
            switch (bindings[{it.first, bindingIndex}].descriptorType) {
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                descriptorWrites[bindingIndex].pBufferInfo = &bufferInfos[{it.first, bindingIndex}];
                break;
            case VK_DESCRIPTOR_TYPE_SAMPLER:
            case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                descriptorWrites[bindingIndex].pImageInfo = _imageInfos[{it.first, bindingIndex}];
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
