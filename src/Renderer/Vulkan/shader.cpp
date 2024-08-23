#include "shader.hpp"
#include "SPIRV-Cross/spirv_cross.hpp"
#include "common.hpp"
#include "descriptors.hpp"
#include "device.hpp"
#include "spirv.hpp"
#include <glslang/Include/ResourceLimits.h>
#include <glslang/MachineIndependent/Versions.h>
#include <glslang/MachineIndependent/localintermediate.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <spirv-tools/libspirv.h>
#include <spirv-tools/libspirv.hpp>
#include <spirv-tools/optimizer.hpp>
#include <stdexcept>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

static std::pair<EShLanguage, VkShaderStageFlagBits> get_stage(std::filesystem::path& shader_path) {
    std::string file_extension = get_file_extension(shader_path.string());
    if (file_extension == "vert") {
        return {EShLangVertex, VK_SHADER_STAGE_VERTEX_BIT};
    } else if (file_extension == "frag") {
        return {EShLangFragment, VK_SHADER_STAGE_FRAGMENT_BIT};
    } else if (file_extension == "comp") {
        return {EShLangCompute, VK_SHADER_STAGE_COMPUTE_BIT};
    } else {
        throw std::runtime_error(shader_path.string() + " has unsupported shader stage type: " + file_extension);
    }
}

static inline VkPipelineBindPoint flag_to_bind_point(VkShaderStageFlagBits stage_flags) {
    switch (stage_flags) {
    case VK_SHADER_STAGE_VERTEX_BIT:
    case VK_SHADER_STAGE_FRAGMENT_BIT:
        return VK_PIPELINE_BIND_POINT_GRAPHICS;
        break;
    case VK_SHADER_STAGE_COMPUTE_BIT:
        return VK_PIPELINE_BIND_POINT_COMPUTE;
        break;
    default:
        throw std::runtime_error("unknown shader stage flag when binding pipeline: " + std::to_string(stage_flags));
    }
}

Shader::Shader(std::filesystem::path file_path, DescriptorLayout* pipeline_descriptor_layout)
    : _path{file_path}, _pipeline_descriptor_layout(pipeline_descriptor_layout) {
    std::string base_cache_path = "assets/cache/shaders/";
    _cache_path = std::filesystem::path(base_cache_path + get_filename(file_path) + ".spv");
    // TODO
    // Compare against file hash
    if (std::filesystem::exists(_cache_path)) {
        if (!read_buffer(_cache_path, _spirv)) {
            throw std::runtime_error("failed to open cached shader: " + _cache_path.string());
        }
        _stage_info.stage = std::get<VkShaderStageFlagBits>(get_stage(_path));
    } else {
        compile();
    }
    create_module();
    // TODO:
    // Should this be before or after compile?
    // Probably after in case a descriptor gets optimized out
    reflect();
}

Shader::~Shader() { vkDestroyShaderModule(Device::device->device->vk_device(), _module, nullptr); }

void Shader::create_module() {
    VkShaderModuleCreateInfo create_info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    create_info.codeSize = _spirv.size() * sizeof(_spirv[0]);
    create_info.pCode = _spirv.data();

    check_result(vkCreateShaderModule(Device::device->vk_device(), &create_info, nullptr, &_module), "failed to create shader module");

    _stage_info.module = _module;
    _stage_info.pName = _entry_point.c_str();
}

void Shader::compile() {
    // TODO:
    // use a semaphore or something similar to allow for multithreaded compilation
    std::unique_lock<std::mutex> _glslang_mutex;
    glslang::InitializeProcess();
    EShLanguage stage;
    std::tie(stage, _stage_info.stage) = get_stage(_path);
    _bind_point = flag_to_bind_point(_stage_info.stage);
    glslang::TShader shader(stage);
    shader.setInvertY(true);
    // NOTE:
    // GL_KHR_vulkan_glsl version is 100
    shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, 100);
    // NOTE:
    // EShTarget must match VK_API_VERSION in device
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_6);

    std::vector<char> shader_code;
    if (!read_buffer(_path, shader_code)) {
        throw std::runtime_error("failed to open shader: " + _path.string());
    }
    const char* shader_string = shader_code.data();
    const int shader_length = shader_code.size();
    const char* shader_names = _path.c_str();
    shader.setStringsWithLengthsAndNames(&shader_string, &shader_length, &shader_names, 1);
    shader.setPreamble(_preamble.c_str());
    shader.setEntryPoint(_entry_point.c_str());

    bool result;

    // TODO
    // Get resource limits from GPU
    const TBuiltInResource* limits = GetDefaultResources();

    int default_version = 460;
    EProfile defaultProfile = ENoProfile;

    bool force_default_version_and_profile = false;
    bool forward_compatible = false;

    EShMessages rules;
    rules = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);
    bool debug = Device::device->enable_validation_layers;
    if (debug) {
        rules = static_cast<EShMessages>(rules | EShMsgDebugInfo);
    }

    // TODO
    // Custom includer
    glslang::TShader::ForbidIncluder includer;

    result = shader.parse(limits, default_version, defaultProfile, force_default_version_and_profile, forward_compatible, rules, includer);
    if (!result) {
        std::string err = "Shader parsing failed for: " + _path.string() + "\n" + shader.getInfoLog();
        if (debug)
            err += shader.getInfoDebugLog();
        throw std::runtime_error(err);
    }

    glslang::TProgram shader_program;
    shader_program.addShader(&shader);
    result = shader_program.link(EShMsgDefault);
    if (!result) {
        std::string err = "Shader linking failed for: " + _path.string() + "\n" + shader_program.getInfoLog();
        if (debug)
            err += shader_program.getInfoDebugLog();
        throw std::runtime_error(err);
    }
    result = shader_program.mapIO();
    if (!result) {
        std::string err = "Shader IO mapping failed for: " + _path.string() + "\n" + shader_program.getInfoLog();
        if (debug)
            err += shader_program.getInfoDebugLog();
        throw std::runtime_error(err);
    }

    glslang::SpvOptions options;

    glslang::GlslangToSpv(*shader_program.getIntermediate(stage), _spirv, &options);
    glslang::FinalizeProcess();

    bool optimize = true;
    if (optimize) {
        // NOTE:
        // Must be consistent with compiler envClient and envTarget versions
        // https://github.com/KhronosGroup/SPIRV-Tools/blob/e5fcb7facf1c891cb30630ea8784da5493d78c22/include/spirv-tools/libspirv.h#L557
        spvtools::Optimizer opt(SPV_ENV_VULKAN_1_3);
        opt.RegisterPerformancePasses();
        opt.RegisterSizePasses();
        if (!opt.Run(_spirv.data(), _spirv.size(), &_spirv)) {
            throw std::runtime_error("Optimization failed for file: " + _path.string());
        }
    }
    if (!write_buffer(_cache_path, _spirv)) {
        std::cout << "WARNING: "
                  << "failed to write shader cache at: " << _cache_path << std::endl;
    }
}

void Shader::reflect() {

    // Generate reflection with spirv-cross
    spirv_cross::Compiler comp(_spirv);
    spirv_cross::ShaderResources res = comp.get_shader_resources();

    /*
    auto loadBuffer = [&](const spirv_cross::Resource& resource, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
        uint32_t set = comp.get_decoration(resource.id, spv::DecorationDescriptorSet);
        uint32_t binding = comp.get_decoration(resource.id, spv::DecorationBinding);
        std::string name = resource.name;
        const spirv_cross::SPIRType& type = comp.get_type(resource.base_type_id);
        // Get size of array if it has 1 element,
        // so size of the array type, like sizeof(array[0])
        size_t size = comp.get_declared_struct_size_runtime_array(type, 1);

        uint32_t bufferExists = globalBuffers.count(name) == 1;
        uint32_t bufferHasCount = bufferCreateInfos.count(name) == 1;
        // NOTE:
        // might be a better way to do this logic
        bool createBuffer = !bufferExists && bufferHasCount;
        // If buffer neither exists nor has a count
        // throw a runtime error
        if (!bufferExists && !bufferHasCount) {
            throw std::runtime_error("missing buffer named: '" + name + " for file: " + path);
        } else if (createBuffer) {
            uint32_t count;
            VkBufferUsageFlags additionalUsage;
            VkMemoryPropertyFlags additionalProperties;
            std::tie(count, additionalUsage, additionalProperties) = bufferCreateInfos.at(name);
            usage |= additionalUsage;
            properties |= additionalProperties;
            globalBuffers[name] = std::make_shared<VulkanBuffer>(_device, size * count, usage, properties);
            if (properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
                globalBuffers.at(name)->map();
            }
        }
        descriptor->addBinding(set, binding, globalBuffers.at(name));
    };
    */

    for (const spirv_cross::Resource& resource : res.storage_buffers) {
        uint32_t set = comp.get_decoration(resource.id, spv::DecorationDescriptorSet);
        uint32_t binding = comp.get_decoration(resource.id, spv::DecorationBinding);
        VkBufferUsageFlags usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        // FIXME:
        // Get property flags from render graph
        VkMemoryPropertyFlags mem_props = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        _pipeline_descriptor_layout->add_binding(set, binding, usage_to_type(usage), stage_info().stage, resource.name, mem_props);
    }
    for (const spirv_cross::Resource& resource : res.uniform_buffers) {
        uint32_t set = comp.get_decoration(resource.id, spv::DecorationDescriptorSet);
        uint32_t binding = comp.get_decoration(resource.id, spv::DecorationBinding);
        VkBufferUsageFlags usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        VkMemoryPropertyFlags mem_props = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        _pipeline_descriptor_layout->add_binding(set, binding, usage_to_type(usage), stage_info().stage, resource.name, mem_props);
    }
    for (const spirv_cross::Resource& resource : res.push_constant_buffers) {
        const spirv_cross::SPIRType& type = comp.get_type(resource.base_type_id);
        size_t size = comp.get_declared_struct_size(type);
        // Name of the type for the push constants
        _push_constants_name = comp.get_name(resource.base_type_id);
        _push_constant_range = {.stageFlags = stage_info().stage, .offset = 0, .size = (uint32_t)size};
    }
    /*
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
            entry.size = size;
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
    */
}
