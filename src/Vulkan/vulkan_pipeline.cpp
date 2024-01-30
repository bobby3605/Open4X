#include "vulkan_pipeline.hpp"
#include "common.hpp"
#include "vulkan_descriptors.hpp"
#include "vulkan_model.hpp"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

GraphicsPipeline::GraphicsPipeline(std::shared_ptr<VulkanDevice> device, VkGraphicsPipelineCreateInfo pipelineInfo,
                                   std::string vertShaderPath, std::string fragShaderPath)
    : VulkanPipeline(device) {
    /*
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstants);
    */

    std::vector<VkDescriptorSetLayout> descriptorLayouts;
    descriptorLayouts.push_back(descriptorManager->descriptors["global"]->getLayout());
    descriptorLayouts.push_back(descriptorManager->descriptors["material"]->getLayout());
    descriptorLayouts.push_back(descriptorManager->descriptors["object"]->getLayout());

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = descriptorLayouts.size();
    pipelineLayoutInfo.pSetLayouts = descriptorLayouts.data();
    // pipelineLayoutInfo.pushConstantRangeCount = 1;
    //  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    checkResult(vkCreatePipelineLayout(_device->device(), &pipelineLayoutInfo, nullptr, &_pipelineLayout),
                "failed to create graphics pipeline layout");

    std::string baseShaderPath = "assets/shaders/";
    std::string baseShaderName = "triangle";

    std::string vertShaderPath = baseShaderPath + baseShaderName + ".vert";
    std::string fragShaderPath = baseShaderPath + baseShaderName + ".frag";

    std::vector<uint32_t> vertShaderCode = compileShader(vertShaderPath);
    std::vector<uint32_t> fragShaderCode = compileShader(fragShaderPath);

    VkShaderModule vertModule = createShaderModule(vertShaderCode);
    VkShaderModule fragModule = createShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;

    checkResult(vkCreateGraphicsPipelines(device->device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline),
                "failed to create graphics pipeline");

    vkDestroyShaderModule(device->device(), vertModule, nullptr);
    vkDestroyShaderModule(device->device(), fragModule, nullptr);
}

void VulkanPipeline::createPipelineLayout(std::vector<VkDescriptorSetLayout>& descriptorLayouts,
                                          std::vector<VkPushConstantRange>& pushConstants) {
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = descriptorLayouts.size();
    pipelineLayoutInfo.pSetLayouts = descriptorLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = pushConstants.size();
    pipelineLayoutInfo.pPushConstantRanges = pushConstants.data();

    checkResult(vkCreatePipelineLayout(_device->device(), &pipelineLayoutInfo, nullptr, &_pipelineLayout),
                "failed to create compute pipeline layout");
}

ComputePipeline::ComputePipeline(std::shared_ptr<VulkanDevice> device, std::vector<VkDescriptorSetLayout>& descriptorLayouts,
                                 std::vector<VkPushConstantRange>& pushConstants, VkPipelineShaderStageCreateInfo stageInfo)
    : VulkanPipeline(device) {

    createPipelineLayout(descriptorLayouts, pushConstants);

    /*
     */
    // NOTE:
    // force subgroup size to maximum
    // only needed on intel
    VkPipelineShaderStageRequiredSubgroupSizeCreateInfo subgroupSizeInfo{};
    subgroupSizeInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO;
    subgroupSizeInfo.requiredSubgroupSize = _device->maxSubgroupSize();

    stageInfo.pNext = &subgroupSizeInfo;

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = _pipelineLayout;

    pipelineInfo.pNext = VK_NULL_HANDLE;
    pipelineInfo.stage = stageInfo;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = 0;

    checkResult(vkCreateComputePipelines(_device->device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline),
                "failed to create compute pipeline");
}

VulkanPipeline::~VulkanPipeline() {
    if (_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(_device->device(), _pipelineLayout, nullptr);
    }
    vkDestroyPipeline(_device->device(), _pipeline, nullptr);
}
