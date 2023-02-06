#include "vulkan_pipeline.hpp"
#include "common.hpp"
#include "vulkan_model.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file: " + filename);
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

VulkanPipeline::VulkanPipeline(VulkanDevice* device, VkGraphicsPipelineCreateInfo pipelineInfo) : device{device} {

    assert(pipelineInfo.layout != nullptr && "Graphics pipeline layout == nullptr");

    auto vertShaderCode = readFile("build/assets/shaders/triangle.vert.spv");
    auto fragShaderCode = readFile("build/assets/shaders/triangle.frag.spv");

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

    checkResult(vkCreatePipelineLayout(device->device(), &pipelineLayoutInfo, nullptr, &_pipelineLayout),
                "failed to create compute pipeline layout");
}

VulkanPipeline::VulkanPipeline(VulkanDevice* device, std::string computeShaderPath, std::vector<VkDescriptorSetLayout>& descriptorLayouts,
                               std::vector<VkPushConstantRange>& pushConstants, VkSpecializationInfo* specializationInfo)
    : device{device} {

    createPipelineLayout(descriptorLayouts, pushConstants);

    VkComputePipelineCreateInfo computePipelineInfo{};
    computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineInfo.layout = _pipelineLayout;

    VkShaderModule computeModule = createShaderModule(readFile(computeShaderPath));

    VkPipelineShaderStageCreateInfo computeStageInfo{};
    computeStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    computeStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    computeStageInfo.module = computeModule;
    computeStageInfo.pName = "main";
    computeStageInfo.pSpecializationInfo = specializationInfo;
    // NOTE:
    // force subgroup size to maximum
    // only needed on intel
    VkPipelineShaderStageRequiredSubgroupSizeCreateInfo subgroupSizeInfo{};
    subgroupSizeInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO;
    subgroupSizeInfo.requiredSubgroupSize = device->maxSubgroupSize();

    computeStageInfo.pNext = &subgroupSizeInfo;

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = _pipelineLayout;

    pipelineInfo.pNext = VK_NULL_HANDLE;
    pipelineInfo.stage = computeStageInfo;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = 0;

    checkResult(vkCreateComputePipelines(device->device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline),
                "failed to create compute pipeline");

    vkDestroyShaderModule(device->device(), computeModule, nullptr);
}

VulkanPipeline::~VulkanPipeline() {
    if (_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device->device(), _pipelineLayout, nullptr);
    }
    vkDestroyPipeline(device->device(), _pipeline, nullptr);
}

VkShaderModule VulkanPipeline::createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule shaderModule;

    checkResult(vkCreateShaderModule(device->device(), &createInfo, nullptr, &shaderModule), "failed to create shader module");

    return shaderModule;
}
