#ifndef VULKAN_PIPELINE_H_
#define VULKAN_PIPELINE_H_

#include "vulkan_device.hpp"
#include <vector>
#include <vulkan/vulkan.h>

class VulkanPipeline {
  public:
    VulkanPipeline(VulkanDevice* device, VkGraphicsPipelineCreateInfo pipelineInfo);
    VulkanPipeline(VulkanDevice* device, std::string computeShaderPath, std::vector<VkDescriptorSetLayout>& descriptorLayouts,
                   std::vector<VkPushConstantRange>& pushConstants, VkSpecializationInfo* specializationInfo);
    ~VulkanPipeline();
    static VkGraphicsPipelineCreateInfo defaultPipelineConfigInfo();
    VkPipeline pipeline() { return _pipeline; }
    VkPipelineLayout pipelineLayout() { return _pipelineLayout; }

  private:
    void createGraphicsPipeline();
    void createComputePipeline();
    VkShaderModule createShaderModule(const std::vector<char>& code);

    void createPipelineLayout(std::vector<VkDescriptorSetLayout>& descriptorLayouts, std::vector<VkPushConstantRange>& pushConstants);

    VulkanDevice* device;
    VkPipeline _pipeline;
    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
};
#endif // VULKAN_PIPELINE_H_
