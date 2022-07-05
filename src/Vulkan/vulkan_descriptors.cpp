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
  pool = createPool();
  globalL = createLayout(globalLayout());
  materialL = createLayout(materialLayout());

}

void VulkanDescriptors::createSets(VkDescriptorSetLayout layout, std::vector<VkDescriptorSet>& sets) {
  for(int i = 0; i < VulkanSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
    sets.push_back(allocateSet(layout));
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

  // createLayout should always be given a unique binding because the unique layouts and number of them is needed to create the pipeline
  // TODO implement a layout cache
  descriptorLayouts.push_back(layout);

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

VkDescriptorSet VulkanDescriptors::allocateSet(VkDescriptorSetLayout layout) {
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

VulkanDescriptors::~VulkanDescriptors() {

  vkDestroyDescriptorPool(device->device(), pool, nullptr);
  for (VkDescriptorSetLayout layout : descriptorLayouts) {
  vkDestroyDescriptorSetLayout(device->device(), layout, nullptr);
  }

}
