#ifndef VULKAN_DESCRIPTORS_H_
#define VULKAN_DESCRIPTORS_H_
#include "vulkan_device.hpp"
#include <vulkan/vulkan_core.h>

class VulkanDescriptors {
  public:
    VulkanDescriptors(VulkanDevice* deviceRef);
    VkDescriptorSetLayout descriptorSetLayout;
    ~VulkanDescriptors();
    std::vector<VkDescriptorSet> descriptorSets;

    VkDescriptorPool createPool();
    VkDescriptorSetLayout
    createLayout(std::vector<VkDescriptorSetLayoutBinding> bindings);
    VkDescriptorSet allocateSet(VkDescriptorSetLayout layout);
    void createSets(VkDescriptorSetLayout layout,
                    std::vector<VkDescriptorSet>& sets);

    VkDescriptorSetLayout getGlobal() const { return globalL; }
    VkDescriptorSetLayout getMaterial() const { return materialL; }

    std::vector<VkDescriptorSetLayout> const& getLayouts() const {
        return descriptorLayouts;
    }

  private:
    void createDescriptorSetLayout();
    void createDescriptorPool();

    std::vector<VkDescriptorSetLayoutBinding> globalLayout();
    std::vector<VkDescriptorSetLayoutBinding> passLayout();
    std::vector<VkDescriptorSetLayoutBinding> materialLayout();
    std::vector<VkDescriptorSetLayoutBinding> objectLayout();

    VulkanDevice* device;

    VkDescriptorPool pool;
    VkDescriptorSetLayout globalL;
    VkDescriptorSetLayout materialL;

    std::vector<VkDescriptorSetLayout> descriptorLayouts;

    std::vector<VkDescriptorType> descriptorTypes = {
        VK_DESCRIPTOR_TYPE_SAMPLER,
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
