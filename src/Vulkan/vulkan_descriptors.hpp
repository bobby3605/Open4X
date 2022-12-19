#ifndef VULKAN_DESCRIPTORS_H_
#define VULKAN_DESCRIPTORS_H_
#include "vulkan_device.hpp"
#include <unordered_map>
#include <vulkan/vulkan_core.h>

class VulkanDescriptors {
  public:
    class VulkanDescriptor {
      public:
        VulkanDescriptor(VulkanDescriptors* descriptorManager, std::string name);
        ~VulkanDescriptor();
        void addBinding(uint32_t bindingID, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, uint32_t descriptorCount = 1);
        void setBindingBuffer(uint32_t bindingID, VkBuffer buffer);
        void createLayout();
        void allocateSet();
        void update();
        VkDescriptorSetLayout getLayout() const { return layout; }
        VkDescriptorSet getSet() const { return set; }

      private:
        VulkanDescriptors* descriptorManager;
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        VkDescriptorSetLayout layout;
        VkDescriptorSet set;
        std::vector<VkDescriptorBufferInfo> bufferInfos;
    };

    VulkanDescriptors(VulkanDevice* deviceRef);
    VkDescriptorSetLayout descriptorSetLayout;
    ~VulkanDescriptors();
    std::vector<VkDescriptorSet> descriptorSets;

    VkDescriptorPool createPool();
    VkDescriptorSetLayout createLayout(std::vector<VkDescriptorSetLayoutBinding> bindings, uint32_t setNum,
                                       std::vector<VkDescriptorSetLayout>& descriptorLayouts);
    VkDescriptorSet allocateSet(VkDescriptorSetLayout layout);
    void createSets(VkDescriptorSetLayout layout, std::vector<VkDescriptorSet>& sets);

    VkDescriptorSetLayout getGlobal() const { return globalL; }
    VkDescriptorSetLayout getObject() const { return objectL; }

    std::vector<VkDescriptorSetLayout> graphicsDescriptorLayouts;

    std::vector<VkDescriptorSetLayoutBinding> materialLayout(uint32_t samplersSize, uint32_t imagesSize, uint32_t normalMapsSize,
                                                             uint32_t metallicRoughnessMapsSize, uint32_t aoMapsSize);

    std::unordered_map<std::string, VulkanDescriptor*> descriptors;

  private:
    void createDescriptorSetLayout();
    void createDescriptorPool();

    std::vector<VkDescriptorSetLayoutBinding> globalLayout();
    std::vector<VkDescriptorSetLayoutBinding> passLayout();
    std::vector<VkDescriptorSetLayoutBinding> objectLayout();

    VulkanDevice* device;

    VkDescriptorPool pool;
    VkDescriptorSetLayout globalL;
    VkDescriptorSetLayout objectL;

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
