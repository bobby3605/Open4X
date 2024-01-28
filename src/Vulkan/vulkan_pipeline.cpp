#include "vulkan_pipeline.hpp"
#include "common.hpp"
#include "vulkan_model.hpp"
#include <cstdint>
#include <fstream>
#include <glslang/Include/ResourceLimits.h>
#include <glslang/MachineIndependent/Versions.h>
#include <glslang/MachineIndependent/localintermediate.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

static EShLanguage getStage(std::string shaderPath) {
    std::string fileExtension = getFileExtension(shaderPath);
    if (fileExtension == "vert") {
        return EShLangVertex;
    } else if (fileExtension == "frag") {
        return EShLangFragment;
    } else if (fileExtension == "comp") {
        return EShLangCompute;
    } else {
        throw std::runtime_error(shaderPath + " has unsupported shader stage type: " + fileExtension);
    }
}

std::vector<uint32_t> VulkanPipeline::compileShader(std::string filePath) {
    glslang::InitializeProcess();
    EShLanguage stage = getStage(filePath);
    glslang::TShader shader(stage);
    shader.setInvertY(true);
    // NOTE:
    // GL_KHR_vulkan_glsl version is 100
    shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, 100);
    // NOTE:
    // EShTarget must match VK_API_VERSION in device
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_6);

    std::vector<char> shaderCode = readFile(filePath);
    const char* shaderString = shaderCode.data();
    const int shaderLength = shaderCode.size();
    const char* shaderNames = filePath.c_str();
    shader.setStringsWithLengthsAndNames(&shaderString, &shaderLength, &shaderNames, 1);
    const char* preamble = "";
    shader.setPreamble(preamble);
    const char* entryPoint = "main";
    shader.setEntryPoint(entryPoint);

    bool result;

    // TODO
    // Get resource limits from GPU
    const TBuiltInResource* limits = GetDefaultResources();

    int defaultVersion = 110;
    EProfile defaultProfile = ENoProfile;

    bool forceDefaultVersionAndProfile = false;
    bool forwardCompatible = false;

    EShMessages rules;
    rules = static_cast<EShMessages>(rules | EShMsgSpvRules | EShMsgVulkanRules);
    bool debug = false;
    if (debug) {
        rules = static_cast<EShMessages>(rules | EShMsgDebugInfo);
    }

    // TODO
    // Custom includer
    glslang::TShader::ForbidIncluder includer;

    result = shader.parse(limits, defaultVersion, defaultProfile, forceDefaultVersionAndProfile, forwardCompatible, rules, includer);
    if (!result) {
        throw std::runtime_error("Shader parsing failed for: " + filePath);
    }

    glslang::TProgram shaderProgram;
    shaderProgram.addShader(&shader);
    result = shaderProgram.link(EShMsgDefault);
    if (!result) {
        throw std::runtime_error("Shader linking failed for: " + filePath);
    }
    result = shaderProgram.mapIO();
    if (!result) {
        throw std::runtime_error("Shader IO mapping failed for: " + filePath);
    }
    shaderProgram.buildReflection();
    for (uint32_t i = 0; i < shaderProgram.getNumBufferVariables(); ++i) {
        glslang::TObjectReflection bufferBindingInfo = shaderProgram.getBufferVariable(i);
        std::cout << "Found buffer named: " << bufferBindingInfo.name << " at binding: " << bufferBindingInfo.getBinding()
                  << " with size: " << bufferBindingInfo.size << std::endl;
    }

    glslang::SpvOptions options;

    std::vector<uint32_t> spirv;
    glslang::GlslangToSpv(*shaderProgram.getIntermediate(stage), spirv, &options);
    glslang::FinalizeProcess();

    return spirv;
}

VulkanPipeline::VulkanPipeline(VulkanDevice* device, VkGraphicsPipelineCreateInfo pipelineInfo) : device{device} {

    assert(pipelineInfo.layout != nullptr && "Graphics pipeline layout == nullptr");

    //    auto vertShaderCode = readFile("build/assets/shaders/triangle.vert.spv");
    //  auto fragShaderCode = readFile("build/assets/shaders/triangle.frag.spv");
    //
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

    std::vector<uint32_t> compShaderCode = compileShader(computeShaderPath);

    VkShaderModule computeModule = createShaderModule(compShaderCode);

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

VkShaderModule VulkanPipeline::createShaderModule(const std::vector<uint32_t>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size() * sizeof(code[0]);
    createInfo.pCode = code.data();
    VkShaderModule shaderModule;

    checkResult(vkCreateShaderModule(device->device(), &createInfo, nullptr, &shaderModule), "failed to create shader module");

    return shaderModule;
}
