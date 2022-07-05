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

VulkanDescriptors::VulkanDescriptors(VulkanDevice *deviceRef) : device{deviceRef} {
  /*
  VkDescriptorPool pool = createPool();
  VkDescriptorSetLayout globalL = createLayout(globalLayout());
  VkDescriptorSetLayout materialL = createLayout(materialLayout());

  std::vector<VkDescriptorSet> globalSets;
  std::vector<VkDescriptorSet> materialSets;

  createSets(pool, globalL, globalSets);
  createSets(pool, materialL, materialSets);
  */

}

void VulkanDescriptors::createSets(VkDescriptorPool pool, VkDescriptorSetLayout layout, std::vector<VkDescriptorSet>& sets) {
  for(int i = 0; i < VulkanSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
    sets.push_back(allocateSet(pool, layout));
  }
}

std::vector<VkDescriptorSetLayoutBinding> VulkanDescriptors::globalLayout(){
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

std::vector<VkDescriptorSetLayoutBinding> VulkanDescriptors::passLayout(){
  std::vector<VkDescriptorSetLayoutBinding> bindings;
  return bindings;
}

std::vector<VkDescriptorSetLayoutBinding> VulkanDescriptors::materialLayout(){
  std::vector<VkDescriptorSetLayoutBinding> bindings;

  VkDescriptorSetLayoutBinding samplerLayoutBinding{};
  samplerLayoutBinding.binding = 0;
  samplerLayoutBinding.descriptorCount = 1;
  samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerLayoutBinding.pImmutableSamplers = nullptr;
  samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  bindings.push_back(samplerLayoutBinding);

  return bindings;
}

std::vector<VkDescriptorSetLayoutBinding> VulkanDescriptors::objectLayout(){
  std::vector<VkDescriptorSetLayoutBinding> bindings;
  return bindings;
}

VkDescriptorSetLayout VulkanDescriptors::createLayout(std::vector<VkDescriptorSetLayoutBinding> bindings) {
  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings = bindings.data();

  VkDescriptorSetLayout layout;
  checkResult(vkCreateDescriptorSetLayout(device->device(), &layoutInfo, nullptr, &layout), "failed to create descriptor set layout");
  return layout;
}

VkDescriptorPool VulkanDescriptors::createPool() {
  int count = 10*VulkanSwapChain::MAX_FRAMES_IN_FLIGHT;
  std::vector<VkDescriptorPoolSize> poolSizes{};
  poolSizes.reserve(descriptorTypes.size());
  for(VkDescriptorType t : descriptorTypes) {
    poolSizes.push_back({t,uint32_t(count)});
  }

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = count*poolSizes.size();

  VkDescriptorPool pool;
  checkResult(vkCreateDescriptorPool(device->device(), &poolInfo, nullptr, &pool),
              "failed to create descriptor pool");
  return pool;
}

VkDescriptorSet VulkanDescriptors::allocateSet(VkDescriptorPool pool, VkDescriptorSetLayout layout) {
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = pool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = &layout;

  VkDescriptorSet set;

  checkResult(vkAllocateDescriptorSets(device->device(), &allocInfo, &set),
              "failed to allocate descriptor sets");
  return set;
}

void VulkanDescriptors::createDescriptorSetLayout() {
  VkDescriptorSetLayoutBinding uboLayoutBinding{};
  uboLayoutBinding.binding = 0;
  uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uboLayoutBinding.descriptorCount = 1;
  uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  uboLayoutBinding.pImmutableSamplers = nullptr;

  VkDescriptorSetLayoutBinding samplerLayoutBinding{};
  samplerLayoutBinding.binding = 1;
  samplerLayoutBinding.descriptorCount = 1;
  samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerLayoutBinding.pImmutableSamplers = nullptr;
  samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};
  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings = bindings.data();

  checkResult(vkCreateDescriptorSetLayout(device->device(), &layoutInfo, nullptr, &descriptorSetLayout),
              "failed to create descriptor set layout");
}

void VulkanDescriptors::createDescriptorPool() {

  int count = 10*VulkanSwapChain::MAX_FRAMES_IN_FLIGHT;
  std::vector<VkDescriptorPoolSize> poolSizes{};
  poolSizes.reserve(descriptorTypes.size());
  for(VkDescriptorType t : descriptorTypes) {
    poolSizes.push_back({t,uint32_t(count)});
  }

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = count*poolSizes.size();

  checkResult(vkCreateDescriptorPool(device->device(), &poolInfo, nullptr, &descriptorPool),
              "failed to create descriptor pool");
}


void VulkanDescriptors::createDescriptorSets(std::vector<VkDescriptorBufferInfo> bufferInfos,
                                             VkImageView textureImageView, VkSampler textureSampler) {
  std::vector<VkDescriptorSetLayout> layouts(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.descriptorSetCount = static_cast<uint32_t>(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);
  allocInfo.pSetLayouts = layouts.data();

  descriptorSets.resize(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);

  checkResult(vkAllocateDescriptorSets(device->device(), &allocInfo, descriptorSets.data()),
              "failed to allocate descriptor sets!");

  uint32_t i = 0;
  for (VkDescriptorBufferInfo bufferInfo : bufferInfos) {
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = textureImageView;
    imageInfo.sampler = textureSampler;

    std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = descriptorSets[i];
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &bufferInfo;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = descriptorSets[i];
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(device->device(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0,
                           nullptr);
    i++;
  }
}

VulkanDescriptors::~VulkanDescriptors() {

  vkDestroyDescriptorPool(device->device(), descriptorPool, nullptr);
  vkDestroyDescriptorSetLayout(device->device(), descriptorSetLayout, nullptr);
}
