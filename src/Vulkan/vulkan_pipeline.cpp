#include "vulkan_pipeline.hpp"
#include "common.hpp"
#include "vulkan_model.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file");
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

    checkResult(vkCreateGraphicsPipelines(device->device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline),
                "failed to create graphics pipeline");

    vkDestroyShaderModule(device->device(), vertModule, nullptr);
    vkDestroyShaderModule(device->device(), fragModule, nullptr);
}

VulkanPipeline::VulkanPipeline(VulkanDevice* device, VkComputePipelineCreateInfo pipelineInfo) : device{device} {
    assert(pipelineInfo.layout != nullptr && "Compute pipeline layout == nullptr");

    auto computeShaderCode = readFile("build/assets/shaders/cull.comp.spv");
    VkShaderModule computeModule = createShaderModule(computeShaderCode);

    VkPipelineShaderStageCreateInfo computeStageInfo{};
    computeStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    computeStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    computeStageInfo.module = computeModule;
    computeStageInfo.pName = "main";

    pipelineInfo.pNext = VK_NULL_HANDLE;
    pipelineInfo.stage = computeStageInfo;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = 0;

    checkResult(vkCreateComputePipelines(device->device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline),
                "failed to create compute pipeline");

    vkDestroyShaderModule(device->device(), computeModule, nullptr);
}

VulkanPipeline::~VulkanPipeline() { vkDestroyPipeline(device->device(), pipeline, nullptr); }

VkShaderModule VulkanPipeline::createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule shaderModule;

    checkResult(vkCreateShaderModule(device->device(), &createInfo, nullptr, &shaderModule), "failed to create shader module");

    return shaderModule;
}
