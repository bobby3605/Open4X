#ifndef VULKAN_DESCRIPTORS_H_
#define VULKAN_DESCRIPTORS_H_
#include "vulkan_buffer.hpp"
#include "vulkan_device.hpp"
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>
#include <utility>
#include <vulkan/vulkan_core.h>

class VulkanDescriptors {
  public:
    class VulkanDescriptor {
      public:
        VulkanDescriptor(VulkanDescriptors* descriptorManager, VkShaderStageFlags stageFlags);
        ~VulkanDescriptor();
        void addBinding(uint32_t setID, uint32_t bindingID, std::vector<VkDescriptorImageInfo>& imageInfos);
        void addBinding(uint32_t setID, uint32_t bindingID, std::shared_ptr<VulkanBuffer> buffer);
        void setImageInfos(uint32_t setID, uint32_t bindingID, std::vector<VkDescriptorImageInfo>* imageInfos);
        std::vector<VkDescriptorSetLayout> getLayouts();
        const std::map<uint32_t, VkDescriptorSet>& getSets() { return sets; }
        void allocateSets();
        void update();
        //        VkDescriptorSetLayout getLayout() const { return layout; }
        //        VkDescriptorSet* getSets() { return sets.data(); }

      private:
        VulkanDescriptors* descriptorManager;
        std::map<std::pair<uint32_t, uint32_t>, VkDescriptorSetLayoutBinding> bindings;
        // Must be map to preserve setID ordering
        // Required because final pipline layout set ordering must be sequential
        std::map<uint32_t, VkDescriptorSetLayout> layouts;
        std::map<uint32_t, VkDescriptorSet> sets;
        std::set<uint32_t> uniqueSetIDs;
        // FIXME
        // make this unique so bufferInfo can be overwritten
        std::map<std::pair<uint32_t, uint32_t>, VkDescriptorBufferInfo*> bufferInfos;
        std::map<std::pair<uint32_t, uint32_t>, std::vector<VkDescriptorImageInfo>*> _imageInfos;
        void createLayout(uint32_t setID);
        VkShaderStageFlags _stageFlags;
    };

    VulkanDescriptors(std::shared_ptr<VulkanDevice> deviceRef);
    ~VulkanDescriptors();

    VkDescriptorPool createPool();
    VulkanDescriptor* createDescriptor(std::string name, VkShaderStageFlags stageFlags);
    std::unordered_map<std::string, VulkanDescriptor*> descriptors;

    static VkDescriptorType getType(VkBufferUsageFlags usageFlags);

    static inline const std::map<VkBufferUsageFlags, VkDescriptorType> usageToTypes = {
        {VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER},
        {VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER},
        {VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER},
        {VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER},
        {VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC},
        {VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC},
    };

    static inline const std::vector<VkDescriptorType> extraTypes = {VK_DESCRIPTOR_TYPE_SAMPLER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                                    VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                                                                    VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT};

  private:
    void createDescriptorSetLayout();
    void createDescriptorPool();

    std::shared_ptr<VulkanDevice> device;

    VkDescriptorPool pool;
};

#endif // VULKAN_DESCRIPTORS_H_
