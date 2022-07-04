#include "vulkan_pipeline.hpp"
#include "common.hpp"
#include "vulkan_model.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

VulkanPipeline::VulkanPipeline(VulkanDevice *deviceRef, VkGraphicsPipelineCreateInfo pipelineInfo)
    : device{deviceRef}, pipelineInfo_{pipelineInfo} {
  createGraphicsPipeline();
}

VulkanPipeline::~VulkanPipeline() { vkDestroyPipeline(device->device(), graphicsPipeline, nullptr); }

VkShaderModule VulkanPipeline::createShaderModule(const std::vector<char> &code) {
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());
  VkShaderModule shaderModule;

  checkResult(vkCreateShaderModule(device->device(), &createInfo, nullptr, &shaderModule),
              "failed to create shader module");

  return shaderModule;
}

static std::vector<char> readFile(const std::string &filename) {
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

void VulkanPipeline::createGraphicsPipeline() {
  assert(pipelineInfo_.pViewportState->pViewports->height != 0 && "Create graphics pipeline viewport height == 0");
  assert(pipelineInfo_.pViewportState->pViewports->width != 0 && "Create graphics pipeline viewport width == 0");
  // TODO: add assert for scissor
  assert(pipelineInfo_.layout != nullptr && "Graphics pipeline layout == nullptr");
  assert(pipelineInfo_.renderPass != nullptr && "Graphics pipeline render pass == nullptr");

  auto vertShaderCode = readFile("build/assets/shaders/triangle.vert.spv");
  auto fragShaderCode = readFile("build/assets/shaders/triangle.frag.spv");

  vertShaderModule = createShaderModule(vertShaderCode);
  fragShaderModule = createShaderModule(fragShaderCode);

  VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
  vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertShaderModule;
  vertShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
  fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragShaderModule;
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

  pipelineInfo_.pStages = shaderStages;
  pipelineInfo_.pVertexInputState = &vertexInputInfo;

  checkResult(
      vkCreateGraphicsPipelines(device->device(), VK_NULL_HANDLE, 1, &pipelineInfo_, nullptr, &graphicsPipeline),
      "failed to create graphics pipeline");

  vkDestroyShaderModule(device->device(), fragShaderModule, nullptr);
  vkDestroyShaderModule(device->device(), vertShaderModule, nullptr);
}