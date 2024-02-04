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
    createInfo.codeSize = spirv.size() * sizeof(spirv[0]);
    createInfo.pCode = spirv.data();

    checkResult(vkCreateShaderModule(_device->device(), &createInfo, nullptr, &shaderModule), "failed to create shader module");

    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = stageFlags;
    std::cout << "created shader module with name: " << name << " and stageFlags: " << stageInfo.stage << std::endl;
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

    std::vector<char> shaderCode = readFile(basePath + path);
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
        std::cout << "Dumping shader reflection for: " << path << std::endl;
        shaderProgram.dumpReflection();
        buffers.reserve(shaderProgram.getNumBufferBlocks() + shaderProgram.getNumUniformBlocks());
        for (uint32_t i = 0; i < shaderProgram.getNumUniformBlocks(); ++i) {
            buffers.push_back(shaderProgram.getUniformBlock(i));
        }
        for (uint32_t i = 0; i < shaderProgram.getNumUniformVariables(); ++i) {
            buffers.push_back(shaderProgram.getUniform(i));
        }
        for (uint32_t i = 0; i < shaderProgram.getNumBufferBlocks(); ++i) {
            buffers.push_back(shaderProgram.getBufferBlock(i));
        }
        for (uint32_t i = 0; i < shaderProgram.getNumBufferVariables(); ++i) {
            buffers.push_back(shaderProgram.getBufferVariable(i));
        }
    } else {
        throw std::runtime_error("Failed to build reflection for: " + path);
    }

    glslang::SpvOptions options;

    glslang::GlslangToSpv(*shaderProgram.getIntermediate(stage), spirv, &options);
    glslang::FinalizeProcess();

    bool optimize = false;
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
                                                           bufferMap& globalBuffers, imageInfosMap& globalImageInfos) {

    for (auto buffer : buffers) {
        auto type = buffer.getType();
        auto qualifier = buffer.getType()->getQualifier();
        if (type->getBasicType() == glslang::EbtSampler) {
            if (globalImageInfos.count(buffer.name) == 1) {
                descriptor->setImageInfos(qualifier.layoutSet, buffer.getBinding(), globalImageInfos[buffer.name]);
            } else {
                throw std::runtime_error("missing image info named: '" + buffer.name + "' for file: " + path);
            }
        } else if (qualifier.isPushConstant()) {
            hasPushConstants = true;
            // If push constant, set values and skip the rest
            pushConstantRange.size = buffer.size;
            pushConstantRange.stageFlags = stageFlags;
        } else if (qualifier.isUniformOrBuffer()) {
            // Check if is a uniform push constant, and skip
            if (buffer.getBinding() == -1) {
                continue;
            }
            auto storageType = qualifier.storage;
            VkDescriptorType descriptorType;
            // If the buffer hasn't been created yet,
            // and a count has been specified,
            // create the buffer
            uint32_t bufferExists = globalBuffers.count(buffer.name) == 1;
            uint32_t bufferHasCount = bufferCounts.count(buffer.name) == 1;
            // NOTE:
            // might be a better way to do this logic
            bool createBuffer = !bufferExists && bufferHasCount;
            // If buffer neither exists nor has a count
            // throw a runtime error
            if (!bufferExists && !bufferHasCount) {
                throw std::runtime_error("missing buffer named: '" + buffer.name +
                                         "' with basic type: " + std::to_string(type->getBasicType()) + " for file: " + path);
            }
            switch (storageType) {
            case glslang::TStorageQualifier::EvqUniform:
                std::cout << "got unifrom buffer" << std::endl;
                descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                if (qualifier.layoutSet == 63) {
                    qualifier.layoutSet = 0;
                }
                if (createBuffer) {
                    globalBuffers[buffer.name] = VulkanBuffer::UniformBuffer(_device, buffer.size * bufferCounts[buffer.name]);
                }
                break;
            case glslang::TStorageQualifier::EvqBuffer:
                std::cout << "got storage buffer" << std::endl;
                descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                if (createBuffer) {
                    // FIXME:
                    // Add a way to set memory flags from the interface
                    globalBuffers[buffer.name] =
                        VulkanBuffer::StorageBuffer(_device, buffer.size * bufferCounts[buffer.name], VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                }
                break;
            default:
                throw std::runtime_error("Unknown storage buffer type when creating descriptor buffers: " +
                                         std::to_string(qualifier.storage));
            }
            std::cout << "Adding binding for buffer: " << buffer.name << std::endl;
            std::cout << "Type: " << descriptorType << std::endl;
            descriptor->addBinding(qualifier.layoutSet, buffer.getBinding(), descriptorType);
            descriptor->setBuffer(qualifier.layoutSet, buffer.getBinding(), globalBuffers[buffer.name]->bufferInfo());
        }
    }

    descriptor->allocateSets();
    descriptor->update();
}

// TODO
// Get specInfo from reflection, so only pData has to be passed
// https://github.com/KhronosGroup/glslang/issues/2011
VulkanRenderGraph::VulkanShader::VulkanShader(std::string path, VkSpecializationInfo* specInfo, std::shared_ptr<VulkanDevice> device)
    : path{path}, _device{device} {
    stageInfo.pSpecializationInfo = specInfo;
    name = getFilename(path);
}

VulkanRenderGraph::VulkanShader::~VulkanShader() {
    if (shaderModule != VK_NULL_HANDLE)
        vkDestroyShaderModule(_device->device(), shaderModule, nullptr);
}
