#ifndef VULKAN_DESCRIPTORS_H_
#define VULKAN_DESCRIPTORS_H_
#include "vulkan_device.hpp"

class VulkanDescriptors {
public:
  VulkanDescriptors(VulkanDevice *deviceRef);
  VkDescriptorSetLayout descriptorSetLayout;
  ~VulkanDescriptors();
  void createDescriptorSets(std::vector<VkDescriptorBufferInfo> bufferInfos, VkImageView textureImageView,
                            VkSampler textureSampler);
  std::vector<VkDescriptorSet> descriptorSets;

private:
  void createDescriptorSetLayout();
  void createDescriptorPool();

  VulkanDevice *device;
  VkDescriptorPool descriptorPool;
};

#endif // VULKAN_DESCRIPTORS_H_
