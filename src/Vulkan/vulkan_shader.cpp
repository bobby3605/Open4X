#include "SPIRV-Cross/spirv_cross.hpp"
#include "common.hpp"
#include "spirv_common.hpp"
#include "vulkan_descriptors.hpp"
#include "vulkan_pipeline.hpp"
#include "vulkan_rendergraph.hpp"
#include <cstdint>
#include <glslang/Include/ResourceLimits.h>
#include <glslang/MachineIndependent/Versions.h>
#include <glslang/MachineIndependent/localintermediate.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <memory>
#include <spirv-tools/libspirv.h>
#include <spirv-tools/libspirv.hpp>
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
                                                           bufferMap& globalBuffers, imageInfosMap& globalImageInfos) {
    // Generate reflection with spirv-cross
    spirv_cross::Compiler comp(spirv);
    spirv_cross::ShaderResources res = comp.get_shader_resources();

    for (const spirv_cross::Resource& resource : res.storage_buffers) {
        uint32_t set = comp.get_decoration(resource.id, spv::DecorationDescriptorSet);
        uint32_t binding = comp.get_decoration(resource.id, spv::DecorationBinding);
        std::string name = resource.name;
        const spirv_cross::SPIRType& type = comp.get_type(resource.base_type_id);
        // Get size of array if it has 1 element,
        // so size of the array type, like sizeof(array[0])
        size_t size = comp.get_declared_struct_size_runtime_array(type, 1);

        // TODO
        // Find a better way to handle this so it doesn't get repeated
        uint32_t bufferExists = globalBuffers.count(name) == 1;
        uint32_t bufferHasCount = bufferCounts.count(name) == 1;
        // NOTE:
        // might be a better way to do this logic
        bool createBuffer = !bufferExists && bufferHasCount;
        // If buffer neither exists nor has a count
        // throw a runtime error
        if (!bufferExists && !bufferHasCount) {
            throw std::runtime_error("missing buffer named: '" + name + " for file: " + path);
        } else if (createBuffer) {
            // FIXME:
            // Add a way to set memory flags from the interface
            globalBuffers[name] = VulkanBuffer::StorageBuffer(_device, size * bufferCounts.at(name), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        }
        descriptor->addBinding(set, binding, globalBuffers.at(name));
    }
    for (const spirv_cross::Resource& resource : res.uniform_buffers) {
        uint32_t set = comp.get_decoration(resource.id, spv::DecorationDescriptorSet);
        uint32_t binding = comp.get_decoration(resource.id, spv::DecorationBinding);
        std::string name = resource.name;
        const spirv_cross::SPIRType& type = comp.get_type(resource.base_type_id);
        size_t size = comp.get_declared_struct_size_runtime_array(type, 1);

        uint32_t bufferExists = globalBuffers.count(name) == 1;
        uint32_t bufferHasCount = bufferCounts.count(name) == 1;
        // NOTE:
        // might be a better way to do this logic
        bool createBuffer = !bufferExists && bufferHasCount;
        // If buffer neither exists nor has a count
        // throw a runtime error
        if (!bufferExists && !bufferHasCount) {
            throw std::runtime_error("missing buffer named: '" + name + " for file: " + path);
        } else if (createBuffer) {
            // FIXME:
            // Add a way to set memory flags from the interface
            globalBuffers[name] = VulkanBuffer::UniformBuffer(_device, size * bufferCounts.at(name));
        }
        descriptor->addBinding(set, binding, globalBuffers.at(name));
    }
    for (const spirv_cross::Resource& resource : res.separate_samplers) {
        uint32_t set = comp.get_decoration(resource.id, spv::DecorationDescriptorSet);
        uint32_t binding = comp.get_decoration(resource.id, spv::DecorationBinding);
        std::string name = resource.name;

        if (globalImageInfos.count(name) == 1) {
            descriptor->setImageInfos(set, binding, globalImageInfos[name]);
        } else {
            throw std::runtime_error("missing image info named: '" + name + "' for file: " + path);
        }
    }
    for (const spirv_cross::Resource& resource : res.separate_images) {
        uint32_t set = comp.get_decoration(resource.id, spv::DecorationDescriptorSet);
        uint32_t binding = comp.get_decoration(resource.id, spv::DecorationBinding);
        std::string name = resource.name;
        if (globalImageInfos.count(name) == 1) {
            descriptor->setImageInfos(set, binding, globalImageInfos[name]);
        } else {
            throw std::runtime_error("missing image info named: '" + name + "' for file: " + path);
        }
    }
    for (const spirv_cross::Resource& resource : res.push_constant_buffers) {
        const spirv_cross::SPIRType& type = comp.get_type(resource.base_type_id);
        size_t size = comp.get_declared_struct_size(type);
        std::string name = resource.name;
        pushConstantRange.size = size;
        pushConstantRange.stageFlags = stageFlags;
        pushConstantRange.offset = 0;
        hasPushConstants = true;
    }
    std::set<uint32_t> uniqueConstantIds;
    uint32_t offset = 0;
    for (const spirv_cross::SpecializationConstant& constant : comp.get_specialization_constants()) {
        hasSpecConstants = true;
        const spirv_cross::SPIRConstant& value = comp.get_constant(constant.id);
        // Only add unique entries
        // Sometimes the constant ids are repeated, for example if it's used as a local size
        if (uniqueConstantIds.count(constant.constant_id) == 0) {
            uniqueConstantIds.insert(constant.constant_id);
            const spirv_cross::SPIRType& type = comp.get_type(value.constant_type);
            size_t size = type.width / 8;

            VkSpecializationMapEntry entry{};
            entry.constantID = constant.constant_id;
            entry.size = type.width / 8;
            entry.offset = offset;
            offset += entry.size;
            specEntries.push_back(entry);
        }
    }
    if (hasSpecConstants) {
        stageInfo.pSpecializationInfo = &specInfo;
        specInfo.mapEntryCount = specEntries.size();
        specInfo.pMapEntries = specEntries.data();
        // offset should be the total size of the spec constants by the end of the loop
        specInfo.dataSize = offset;
    }

    descriptor->allocateSets();
    descriptor->update();
}

VulkanRenderGraph::VulkanShader::VulkanShader(std::string path, std::shared_ptr<VulkanDevice> device) : path{path}, _device{device} {
    name = getFilename(path);
}

VulkanRenderGraph::VulkanShader::~VulkanShader() {
    if (shaderModule != VK_NULL_HANDLE)
        vkDestroyShaderModule(_device->device(), shaderModule, nullptr);
}
