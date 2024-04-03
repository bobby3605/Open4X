#include "vulkan_descriptors.hpp"
#include "common.hpp"
#include "stb/stb_image.h"
#include "vulkan_swapchain.hpp"
#include <cassert>
#include <cstdint>
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

void VulkanDescriptors::VulkanDescriptor::addBinding(uint32_t setID, uint32_t bindingID, std::shared_ptr<VulkanBuffer> buffer) {

    VkDescriptorSetLayoutBinding binding{};
    binding.binding = bindingID;
    binding.descriptorType = getType(buffer->usageFlags());
    binding.descriptorCount = 1;
    binding.pImmutableSamplers = nullptr;
    binding.stageFlags = _stageFlags;

    bindings.insert({{setID, bindingID}, binding});
    bufferInfos.insert({{setID, bindingID}, buffer->bufferInfo()});
    uniqueSetIDs.insert(setID);
}

void VulkanDescriptors::VulkanDescriptor::setImageInfos(uint32_t setID, uint32_t bindingID,
                                                        std::vector<VkDescriptorImageInfo>* imageInfos) {

    VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;

    if (imageInfos->at(0).imageView != VK_NULL_HANDLE) {
        descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    }
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = bindingID;
    binding.descriptorType = descriptorType;
    binding.descriptorCount = imageInfos->size();
    binding.pImmutableSamplers = nullptr;
    binding.stageFlags = _stageFlags;

    bindings.insert({{setID, bindingID}, binding});
    _imageInfos.insert({{setID, bindingID}, imageInfos});
    uniqueSetIDs.insert(setID);
}

void VulkanDescriptors::VulkanDescriptor::createLayout(uint32_t setID) {
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    std::vector<VkDescriptorSetLayoutBinding> _bindings;
    for (auto b : bindings) {
        if (b.first.first == setID) {
            _bindings.push_back(b.second);
        }
    }
    layoutInfo.pBindings = _bindings.data();
    layoutInfo.bindingCount = static_cast<uint32_t>(_bindings.size());

    VkDescriptorSetLayout layout;
    checkResult(vkCreateDescriptorSetLayout(descriptorManager->device->device(), &layoutInfo, nullptr, &layout),
                "failed to create descriptor set layout");
    layouts.insert({setID, layout});
}

std::vector<VkDescriptorSetLayout> VulkanDescriptors::VulkanDescriptor::getLayouts() {
    std::vector<VkDescriptorSetLayout> flatLayouts;
    for (auto layout : layouts) {
        flatLayouts.push_back(layout.second);
    }
    return flatLayouts;
}

void VulkanDescriptors::VulkanDescriptor::allocateSets() {
    for (auto uniqueSetID : uniqueSetIDs) {
        createLayout(uniqueSetID);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorManager->pool;
        allocInfo.descriptorSetCount = 1;
        VkDescriptorSetLayout layout = layouts.at(uniqueSetID);
        allocInfo.pSetLayouts = &layout;

        VkDescriptorSet set;
        // TODO
        // allocate multiple sets at once
        checkResult(vkAllocateDescriptorSets(descriptorManager->device->device(), &allocInfo, &set), "failed to allocate descriptor sets");
        sets[uniqueSetID] = set;
    }
}

void VulkanDescriptors::VulkanDescriptor::update() {
    std::vector<VkWriteDescriptorSet> descriptorWrites;
    for (auto binding : bindings) {
        uint32_t setIndex = binding.first.first;
        uint32_t bindingIndex = binding.first.second;
        VkDescriptorSetLayoutBinding actualBinding = binding.second;

        assert(bindingIndex == actualBinding.binding);

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = sets.at(setIndex);
        descriptorWrite.dstBinding = actualBinding.binding;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = actualBinding.descriptorType;
        // NOTE:
        // might only support descriptor count of 1
        descriptorWrite.descriptorCount = actualBinding.descriptorCount;
        switch (actualBinding.descriptorType) {
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            descriptorWrite.pBufferInfo = bufferInfos.at({setIndex, bindingIndex});
            break;
        case VK_DESCRIPTOR_TYPE_SAMPLER:
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            descriptorWrite.pImageInfo = _imageInfos.at({setIndex, bindingIndex})->data();
            break;
        default:
            throw std::runtime_error("unknown descriptor type");
        }
        descriptorWrites.push_back(descriptorWrite);
    }
    vkUpdateDescriptorSets(descriptorManager->device->device(), descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

VulkanDescriptors::VulkanDescriptors(std::shared_ptr<VulkanDevice> deviceRef) : device{deviceRef} { pool = createPool(); }

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
