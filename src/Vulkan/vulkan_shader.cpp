#include "Include/BaseTypes.h"
#include "common.hpp"
#include "vulkan_descriptors.hpp"
#include "vulkan_pipeline.hpp"
#include "vulkan_rendergraph.hpp"
#include <glslang/Include/ResourceLimits.h>
#include <glslang/MachineIndependent/Versions.h>
#include <glslang/MachineIndependent/localintermediate.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <memory>
#include <spirv-tools/libspirv.h>
#include <spirv-tools/optimizer.hpp>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

static std::pair<EShLanguage, VkShaderStageFlagBits> getStage(std::string shaderPath) {
    std::string fileExtension = getFileExtension(shaderPath);
    if (fileExtension == "vert") {
        return {EShLangVertex, VK_SHADER_STAGE_VERTEX_BIT};
    } else if (fileExtension == "frag") {
        return {EShLangFragment, VK_SHADER_STAGE_FRAGMENT_BIT};
    } else if (fileExtension == "comp") {
        return {EShLangCompute, VK_SHADER_STAGE_COMPUTE_BIT};
    } else {
        throw std::runtime_error(shaderPath + " has unsupported shader stage type: " + fileExtension);
    }
}

static inline VkPipelineBindPoint flagToBindPoint(VkShaderStageFlagBits stageFlags) {
    // TODO
    // Only run this switch once, not every frame
    switch (stageFlags) {
    case VK_SHADER_STAGE_VERTEX_BIT:
    case VK_SHADER_STAGE_FRAGMENT_BIT:
        return VK_PIPELINE_BIND_POINT_GRAPHICS;
        break;
    case VK_SHADER_STAGE_COMPUTE_BIT:
        return VK_PIPELINE_BIND_POINT_COMPUTE;
        break;
    default:
        throw std::runtime_error("unknown shader stage flag when binding pipeline: " + std::to_string(stageFlags));
    }
}

void VulkanRenderGraph::VulkanShader::createShaderModule() {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size() * sizeof(code[0]);
    createInfo.pCode = code.data();

    checkResult(vkCreateShaderModule(_device->device(), &createInfo, nullptr, &shaderModule), "failed to create shader module");

    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = stageFlags;
    stageInfo.module = shaderModule;
    stageInfo.pName = _entryPoint.c_str();
}

void VulkanRenderGraph::VulkanShader::compile() {
    // FIXME
    // put a mutex around this to allow for multi-threaded compilation
    glslang::InitializeProcess();
    EShLanguage stage;
    std::tie(stage, stageFlags) = getStage(path);
    bindPoint = flagToBindPoint(stageFlags);
    glslang::TShader shader(stage);
    shader.setInvertY(true);
    // NOTE:
    // GL_KHR_vulkan_glsl version is 100
    shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, 100);
    // NOTE:
    // EShTarget must match VK_API_VERSION in device
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_6);

    std::vector<char> shaderCode = readFile(path);
    const char* shaderString = shaderCode.data();
    const int shaderLength = shaderCode.size();
    const char* shaderNames = path.c_str();
    shader.setStringsWithLengthsAndNames(&shaderString, &shaderLength, &shaderNames, 1);
    shader.setPreamble(_preamble.c_str());
    shader.setEntryPoint(_entryPoint.c_str());

    bool result;

    // TODO
    // Get resource limits from GPU
    const TBuiltInResource* limits = GetDefaultResources();

    int defaultVersion = 460;
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
        throw std::runtime_error("Shader parsing failed for: " + path);
    }

    glslang::TProgram shaderProgram;
    shaderProgram.addShader(&shader);
    result = shaderProgram.link(EShMsgDefault);
    if (!result) {
        throw std::runtime_error("Shader linking failed for: " + path);
    }
    result = shaderProgram.mapIO();
    if (!result) {
        throw std::runtime_error("Shader IO mapping failed for: " + path);
    }

    if (shaderProgram.buildReflection(EShReflectionSeparateBuffers)) {
        buffers.reserve(shaderProgram.getNumBufferBlocks() + shaderProgram.getNumUniformBlocks());
        for (uint32_t i = 0; i < shaderProgram.getNumUniformBlocks(); ++i) {
            buffers.push_back(shaderProgram.getUniformBlock(i));
        }
        for (uint32_t i = 0; i < shaderProgram.getNumBufferBlocks(); ++i) {
            buffers.push_back(shaderProgram.getBufferBlock(i));
        }
    } else {
        throw std::runtime_error("Failed to build reflection for: " + path);
    }

    glslang::SpvOptions options;

    glslang::GlslangToSpv(*shaderProgram.getIntermediate(stage), spirv, &options);
    glslang::FinalizeProcess();

    bool optimize = true;
    if (optimize) {
        // NOTE:
        // Must be consistent with compiler envClient and envTarget versions
        // https://github.com/KhronosGroup/SPIRV-Tools/blob/e5fcb7facf1c891cb30630ea8784da5493d78c22/include/spirv-tools/libspirv.h#L557
        spvtools::Optimizer opt(SPV_ENV_VULKAN_1_3);
        opt.RegisterPerformancePasses();
        opt.RegisterSizePasses();
        if (!opt.Run(spirv.data(), spirv.size(), &spirv)) {
            throw std::runtime_error("Optimization failed for file: " + path);
        }
    }
}

void VulkanRenderGraph::VulkanShader::setDescriptorBuffers(VulkanDescriptors::VulkanDescriptor* descriptor, bufferCountMap& bufferCounts,
                                                           bufferMap& globalBuffers) {
    for (auto buffer : buffers) {
        if (globalBuffers.count(buffer.name) == 0) {
            if (bufferCounts.count(buffer.name) != 0) {
                switch (buffer.getType()->getQualifier().storage) {
                case glslang::TStorageQualifier::EvqUniform:
                    if (buffer.getType()->getQualifier().isPushConstant()) {
                        pushConstantRange.size = buffer.size;
                        pushConstantRange.stageFlags = stageFlags;
                    } else {
                        descriptor->addBinding(buffer.getType()->getQualifier().layoutSet, buffer.getBinding(),
                                               VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
                        globalBuffers[buffer.name] = VulkanBuffer::UniformBuffer(_device, buffer.size * bufferCounts[buffer.name]);
                    }
                    break;
                case glslang::TStorageQualifier::EvqBuffer:
                    descriptor->addBinding(buffer.getType()->getQualifier().layoutSet, buffer.getBinding(),
                                           VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
                    // FIXME:
                    // Add a way to set memory flags from the interface
                    globalBuffers[buffer.name] =
                        VulkanBuffer::StorageBuffer(_device, buffer.size * bufferCounts[buffer.name], VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                    break;
                default:
                    throw std::runtime_error("Unknown storage buffer type when creating descriptor buffers: " +
                                             std::to_string(buffer.getType()->getQualifier().storage));
                }
            } else {
                throw std::runtime_error("missing buffer named: " + buffer.name + " for file: " + path);
            }
        }
        // Don't need to check if globalBuffers[buffer.name] exists because an exception would have been thrown above if it doesn't exist
        descriptor->setBuffer(buffer.getType()->getQualifier().layoutSet, buffer.getBinding(), globalBuffers[buffer.name]->bufferInfo());
    }
    descriptor->allocateSets();
    descriptor->update();
}

VulkanRenderGraph::VulkanShader::VulkanShader(std::string path, VkSpecializationInfo* specInfo) : path{path} {
    stageInfo.pSpecializationInfo = specInfo;
    name = getFilename(path);
}

VulkanRenderGraph::VulkanShader::~VulkanShader() { vkDestroyShaderModule(_device->device(), shaderModule, nullptr); }
