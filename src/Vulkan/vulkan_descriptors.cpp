#include "vulkan_descriptors.hpp"
#include "../../external/stb_image.h"
#include "common.hpp"
#include "vulkan_swapchain.hpp"
#include <array>
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
                                                     uint32_t descriptorCount) {
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = bindingID;
    binding.descriptorType = descriptorType;
    binding.descriptorCount = descriptorCount;
    binding.pImmutableSamplers = nullptr;
    binding.stageFlags = stageFlags;

    if (bindings.size() < (bindingID + 1)) {
        bindings.resize(bindingID + 1);
        bufferInfos.resize(bindingID + 1);
    }
    bindings[bindingID] = binding;
}

void VulkanDescriptors::VulkanDescriptor::setBindingBuffer(uint32_t bindingID, VkBuffer buffer) {
    bufferInfos[bindingID].buffer = buffer;
    bufferInfos[bindingID].offset = 0;
    bufferInfos[bindingID].range = VK_WHOLE_SIZE;
}

void VulkanDescriptors::VulkanDescriptor::createLayout() {
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    checkResult(vkCreateDescriptorSetLayout(descriptorManager->device->device(), &layoutInfo, nullptr, &layout),
                "failed to create descriptor set layout");
}

void VulkanDescriptors::VulkanDescriptor::allocateSet() {

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorManager->pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    checkResult(vkAllocateDescriptorSets(descriptorManager->device->device(), &allocInfo, &set), "failed to allocate descriptor sets");
}

void VulkanDescriptors::VulkanDescriptor::update() {

    std::vector<VkWriteDescriptorSet> descriptorWrites(bindings.size());
    for (uint32_t i = 0; i < bindings.size(); ++i) {
        descriptorWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[i].dstSet = set;
        descriptorWrites[i].dstBinding = bindings[i].binding;
        descriptorWrites[i].dstArrayElement = 0;
        descriptorWrites[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[i].descriptorCount = 1;
        descriptorWrites[i].pBufferInfo = &bufferInfos[i];
    }
    vkUpdateDescriptorSets(descriptorManager->device->device(), bindings.size(), descriptorWrites.data(), 0, nullptr);
}

VulkanDescriptors::VulkanDescriptors(VulkanDevice* deviceRef) : device{deviceRef} {
    pool = createPool();
    globalL = createLayout(globalLayout(), 0, graphicsDescriptorLayouts);
    objectL = createLayout(objectLayout(), 2, graphicsDescriptorLayouts);
}

void VulkanDescriptors::createSets(VkDescriptorSetLayout layout, std::vector<VkDescriptorSet>& sets) {
    for (int i = 0; i < VulkanSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        sets.push_back(allocateSet(layout));
    }
}

std::vector<VkDescriptorSetLayoutBinding> VulkanDescriptors::globalLayout() {
    std::vector<VkDescriptorSetLayoutBinding> bindings;

    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;
    bindings.push_back(uboLayoutBinding);

    return bindings;
}

std::vector<VkDescriptorSetLayoutBinding> VulkanDescriptors::passLayout() {
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    return bindings;
}

std::vector<VkDescriptorSetLayoutBinding> VulkanDescriptors::materialLayout(uint32_t samplersSize, uint32_t imagesSize,
                                                                            uint32_t normalMapsSize, uint32_t metallicRoughnessMapsSize,
                                                                            uint32_t aoMapsSize) {
    std::vector<VkDescriptorSetLayoutBinding> bindings;

    VkDescriptorSetLayoutBinding samplersBufferLayoutBinding{};
    samplersBufferLayoutBinding.binding = 0;
    samplersBufferLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    samplersBufferLayoutBinding.descriptorCount = samplersSize;
    samplersBufferLayoutBinding.pImmutableSamplers = nullptr;
    samplersBufferLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings.push_back(samplersBufferLayoutBinding);

    VkDescriptorSetLayoutBinding imagesBufferLayoutBinding{};
    imagesBufferLayoutBinding.binding = 1;
    imagesBufferLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    imagesBufferLayoutBinding.descriptorCount = imagesSize;
    imagesBufferLayoutBinding.pImmutableSamplers = nullptr;
    imagesBufferLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings.push_back(imagesBufferLayoutBinding);

    VkDescriptorSetLayoutBinding normalMapsBufferLayoutBinding{};
    normalMapsBufferLayoutBinding.binding = 2;
    normalMapsBufferLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    normalMapsBufferLayoutBinding.descriptorCount = normalMapsSize;
    normalMapsBufferLayoutBinding.pImmutableSamplers = nullptr;
    normalMapsBufferLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings.push_back(normalMapsBufferLayoutBinding);

    VkDescriptorSetLayoutBinding metallicRoughnessBufferLayoutBinding{};
    metallicRoughnessBufferLayoutBinding.binding = 3;
    metallicRoughnessBufferLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    metallicRoughnessBufferLayoutBinding.descriptorCount = metallicRoughnessMapsSize;
    metallicRoughnessBufferLayoutBinding.pImmutableSamplers = nullptr;
    metallicRoughnessBufferLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings.push_back(metallicRoughnessBufferLayoutBinding);

    VkDescriptorSetLayoutBinding aoMapsBufferLayoutBinding{};
    aoMapsBufferLayoutBinding.binding = 4;
    aoMapsBufferLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    aoMapsBufferLayoutBinding.descriptorCount = aoMapsSize;
    aoMapsBufferLayoutBinding.pImmutableSamplers = nullptr;
    aoMapsBufferLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings.push_back(aoMapsBufferLayoutBinding);
    return bindings;
}

std::vector<VkDescriptorSetLayoutBinding> VulkanDescriptors::objectLayout() {
    std::vector<VkDescriptorSetLayoutBinding> bindings;

    VkDescriptorSetLayoutBinding SSBOLayoutBinding{};
    SSBOLayoutBinding.binding = 0;
    SSBOLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    SSBOLayoutBinding.descriptorCount = 1;
    SSBOLayoutBinding.pImmutableSamplers = nullptr;
    SSBOLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    bindings.push_back(SSBOLayoutBinding);

    VkDescriptorSetLayoutBinding materialBufferLayoutBinding{};
    materialBufferLayoutBinding.binding = 2;
    materialBufferLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    materialBufferLayoutBinding.descriptorCount = 1;
    materialBufferLayoutBinding.pImmutableSamplers = nullptr;
    materialBufferLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    bindings.push_back(materialBufferLayoutBinding);

    VkDescriptorSetLayoutBinding culledInstanceIndicesBufferLayoutBinding{};
    culledInstanceIndicesBufferLayoutBinding.binding = 3;
    culledInstanceIndicesBufferLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    culledInstanceIndicesBufferLayoutBinding.descriptorCount = 1;
    culledInstanceIndicesBufferLayoutBinding.pImmutableSamplers = nullptr;
    culledInstanceIndicesBufferLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    bindings.push_back(culledInstanceIndicesBufferLayoutBinding);

    VkDescriptorSetLayoutBinding materialIndicesBufferLayoutBinding{};
    materialIndicesBufferLayoutBinding.binding = 4;
    materialIndicesBufferLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    materialIndicesBufferLayoutBinding.descriptorCount = 1;
    materialIndicesBufferLayoutBinding.pImmutableSamplers = nullptr;
    materialIndicesBufferLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    bindings.push_back(materialIndicesBufferLayoutBinding);

    return bindings;
}

VkDescriptorSetLayout VulkanDescriptors::createLayout(std::vector<VkDescriptorSetLayoutBinding> bindings, uint32_t setNum,
                                                      std::vector<VkDescriptorSetLayout>& descriptorLayouts) {
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    VkDescriptorSetLayout layout;
    checkResult(vkCreateDescriptorSetLayout(device->device(), &layoutInfo, nullptr, &layout), "failed to create descriptor set layout");

    // createLayout should always be given a unique binding because the unique
    // layouts and number of them is needed to create the pipeline
    // TODO implement a layout cache

    // NOTE:
    // For some reason, insert doesn't work here, it looks like the pointer gets mangled in gdb for objectSet
    // only resize if it's a greater size
    if ((setNum + 1) > descriptorLayouts.size()) {
        descriptorLayouts.resize(setNum + 1);
    }
    descriptorLayouts[setNum] = layout;

    return layout;
}

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

VkDescriptorSet VulkanDescriptors::allocateSet(VkDescriptorSetLayout layout) {
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    VkDescriptorSet set;

    checkResult(vkAllocateDescriptorSets(device->device(), &allocInfo, &set), "failed to allocate descriptor sets");
    return set;
}

VulkanDescriptors::~VulkanDescriptors() {

    vkDestroyDescriptorPool(device->device(), pool, nullptr);
    for (VkDescriptorSetLayout layout : graphicsDescriptorLayouts) {
        vkDestroyDescriptorSetLayout(device->device(), layout, nullptr);
    }
}
