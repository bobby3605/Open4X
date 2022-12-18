#ifndef VULKAN_PIPELINE_H_
#define VULKAN_PIPELINE_H_

#include "vulkan_device.hpp"
#include <vector>
#include <vulkan/vulkan.h>

class VulkanPipeline {
  public:
    VulkanPipeline(VulkanDevice* device, VkGraphicsPipelineCreateInfo pipelineInfo);
    VulkanPipeline(VulkanDevice* device, VkComputePipelineCreateInfo pipelineInfo);
    ~VulkanPipeline();
    static VkGraphicsPipelineCreateInfo defaultPipelineConfigInfo();
    VkPipeline getPipeline() { return pipeline; }

  private:
    void createGraphicsPipeline();
    void createComputePipeline();
    VkShaderModule createShaderModule(const std::vector<char>& code);

    VulkanDevice* device;
    VkPipeline pipeline;
};
#endif // VULKAN_PIPELINE_H_
