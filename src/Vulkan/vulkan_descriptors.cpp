#include "vulkan_descriptors.hpp"
#include "../../external/stb_image.h"
#include "common.hpp"
#include "vulkan_swapchain.hpp"
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>
#include <vulkan/vulkan_core.h>

VulkanDescriptors::VulkanDescriptor::VulkanDescriptor(VulkanDescriptors* descriptorManager, std::string name)
    : descriptorManager(descriptorManager) {
    descriptorManager->descriptors.insert({name, this});
}

VulkanDescriptors::VulkanDescriptor::~VulkanDescriptor() {
    vkDestroyDescriptorSetLayout(descriptorManager->device->device(), layout, nullptr);
}

void VulkanDescriptors::VulkanDescriptor::addBinding(uint32_t bindingID, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags,
                                                     std::vector<VkDescriptorImageInfo>& imageInfos, uint32_t setID) {
    if (setID == 0) {
        VkDescriptorSetLayoutBinding binding{};
        binding.binding = bindingID;
        binding.descriptorType = descriptorType;
        binding.descriptorCount = imageInfos.size();
        binding.pImmutableSamplers = nullptr;
        binding.stageFlags = stageFlags;

        bindings.insert({bindingID, binding});
    }

    _imageInfos.insert({{setID, bindingID}, imageInfos.data()});
}

void VulkanDescriptors::VulkanDescriptor::addBinding(uint32_t bindingID, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags,
                                                     VkBuffer buffer, uint32_t setID) {
    if (setID == 0) {
        VkDescriptorSetLayoutBinding binding{};
        binding.binding = bindingID;
        binding.descriptorType = descriptorType;
        binding.descriptorCount = 1;
        binding.pImmutableSamplers = nullptr;
        binding.stageFlags = stageFlags;

        bindings.insert({bindingID, binding});
    }

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = VK_WHOLE_SIZE;

    bufferInfos.insert({{setID, bindingID}, bufferInfo});
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

VkDescriptorPool VulkanDescriptors::createPool() {
    int count = 10 * VulkanSwapChain::MAX_FRAMES_IN_FLIGHT;
    std::vector<VkDescriptorPoolSize> poolSizes{};
    poolSizes.reserve(descriptorTypes.size());
    for (VkDescriptorType t : descriptorTypes) {
        poolSizes.push_back({t, uint32_t(count)});
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

VulkanDescriptors::~VulkanDescriptors() { vkDestroyDescriptorPool(device->device(), pool, nullptr); }
