#ifndef VULKAN_DESCRIPTORS_H_
#define VULKAN_DESCRIPTORS_H_
#include "vulkan_device.hpp"
#include <map>
#include <utility>
#include <vulkan/vulkan_core.h>

class VulkanDescriptors {
  public:
    class VulkanDescriptor {
      public:
        VulkanDescriptor(VulkanDescriptors* descriptorManager, std::string name);
        ~VulkanDescriptor();
        void addBinding(uint32_t bindingID, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags,
                        std::vector<VkDescriptorImageInfo>& imageInfos, uint32_t setID = 0);
        void addBinding(uint32_t bindingID, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, VkBuffer buffer,
                        uint32_t setID = 0);
        void allocateSets(uint32_t count = 1);
        void update();
        VkDescriptorSetLayout getLayout() const { return layout; }
        VkDescriptorSet* getSets() { return sets.data(); }

      private:
        VulkanDescriptors* descriptorManager;
        std::map<uint32_t, VkDescriptorSetLayoutBinding> bindings;
        VkDescriptorSetLayout layout;
        std::vector<VkDescriptorSet> sets;
        std::map<std::pair<uint32_t, uint32_t>, VkDescriptorBufferInfo> bufferInfos;
        std::map<std::pair<uint32_t, uint32_t>, VkDescriptorImageInfo*> _imageInfos;
        void createLayout();
    };

    VulkanDescriptors(VulkanDevice* deviceRef);
    ~VulkanDescriptors();

    VkDescriptorPool createPool();
    std::unordered_map<std::string, VulkanDescriptor*> descriptors;

  private:
    void createDescriptorSetLayout();
    void createDescriptorPool();

    VulkanDevice* device;

    VkDescriptorPool pool;

    std::vector<VkDescriptorType> descriptorTypes = {VK_DESCRIPTOR_TYPE_SAMPLER,
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
