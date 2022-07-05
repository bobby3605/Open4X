#ifndef VULKAN_DESCRIPTORS_H_
#define VULKAN_DESCRIPTORS_H_
#include "vulkan_device.hpp"
#include <vulkan/vulkan_core.h>

class VulkanDescriptors {
public:
  VulkanDescriptors(VulkanDevice *deviceRef);
  VkDescriptorSetLayout descriptorSetLayout;
  ~VulkanDescriptors();
  void createDescriptorSets(std::vector<VkDescriptorBufferInfo> bufferInfos, VkImageView textureImageView,
                            VkSampler textureSampler);
  std::vector<VkDescriptorSet> descriptorSets;

    VkDescriptorPool createPool();
VkDescriptorSetLayout createLayout(std::vector<VkDescriptorSetLayoutBinding> bindings);
VkDescriptorSet allocateSet(VkDescriptorPool pool, VkDescriptorSetLayout layout);
void createSets(VkDescriptorPool pool, VkDescriptorSetLayout layout, std::vector<VkDescriptorSet>& sets);

std::vector<VkDescriptorSetLayoutBinding> globalLayout();
std::vector<VkDescriptorSetLayoutBinding> passLayout();
std::vector<VkDescriptorSetLayoutBinding> materialLayout();
std::vector<VkDescriptorSetLayoutBinding> objectLayout();


private:
  void createDescriptorSetLayout();
void createDescriptorPool();

  VulkanDevice *device;
  VkDescriptorPool descriptorPool;

    std::vector<VkDescriptorType> descriptorTypes = {VK_DESCRIPTOR_TYPE_SAMPLER ,
    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
    VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
    VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
    VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
    VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT};

};

#endif // VULKAN_DESCRIPTORS_H_
