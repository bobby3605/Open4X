#ifndef VULKAN_PIPELINE_H_
#define VULKAN_PIPELINE_H_

#include "vulkan_device.hpp"
#include <vector>
#include <vulkan/vulkan.h>

class VulkanPipeline {
public:
  VulkanPipeline(VulkanDevice *deviceRef, VkGraphicsPipelineCreateInfo pipelineInfo);
  ~VulkanPipeline();
  static VkGraphicsPipelineCreateInfo defaultPipelineConfigInfo();
  VkPipeline getPipeline() { return graphicsPipeline; }

private:
  void createGraphicsPipeline();
  VkShaderModule createShaderModule(const std::vector<char> &code);

  VulkanDevice *device;
  VkPipeline graphicsPipeline;
  VkShaderModule vertShaderModule;
  VkShaderModule fragShaderModule;

  VkGraphicsPipelineCreateInfo pipelineInfo_;
};
#endif // VULKAN_PIPELINE_H_

/*
VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
pipelineLayoutInfo.setLayoutCount = 1;
pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
pipelineLayoutInfo.pushConstantRangeCount = 0;
pipelineLayoutInfo.pPushConstantRanges = nullptr;

checkResult(vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout),
            "failed to create pipeline layout");

            */
