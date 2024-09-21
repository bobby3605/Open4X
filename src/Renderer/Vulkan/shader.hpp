#ifndef SHADER_H_
#define SHADER_H_
#include "descriptors.hpp"
#include <filesystem>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

class CompilerCache {
    std::vector<char> _file_data;
    bool _is_cached = false;

  public:
    CompilerCache(std::filesystem::path file_path);
    std::vector<char> const& file_data() const { return _file_data; }
    const bool is_cached() const { return _is_cached; }
};

class Shader {
  public:
    Shader(std::filesystem::path file_path, DescriptorLayout* pipeline_descriptor_layout, void* specialization_data = nullptr);
    ~Shader();
    VkPipelineBindPoint bind_point() const { return _bind_point; }
    VkPipelineShaderStageCreateInfo stage_info() const { return _stage_info; }
    bool has_push_constants() const { return _push_constant_range.has_value(); }
    VkPushConstantRange const& push_constant_range() const { return _push_constant_range.value(); };
    std::string const& push_constants_name() const { return _push_constants_name; };
    std::unordered_map<std::string, size_t> const& push_constant_offsets() const { return _push_constant_offsets; }
    VkVertexInputBindingDescription const& binding_description() const { return _binding_description; }
    std::vector<VkVertexInputAttributeDescription> const& attribute_descriptions() const { return _attribute_descriptions; }

  private:
    CompilerCache _compiler_cache;
    std::filesystem::path _path;
    std::filesystem::path _cache_path;
    std::vector<uint32_t> _spirv;
    std::string _preamble = "";
    std::string _entry_point = "main";
    VkPipelineBindPoint _bind_point;
    void compile();
    VkShaderModule _module;
    VkPipelineShaderStageCreateInfo _stage_info{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    void create_module();
    void reflect();
    DescriptorLayout* _pipeline_descriptor_layout;
    std::optional<VkPushConstantRange> _push_constant_range;
    std::string _push_constants_name;
    std::unordered_map<std::string, size_t> _push_constant_offsets;
    std::mutex _glslang_mutex;
    std::vector<VkSpecializationMapEntry> _spec_entries;
    VkSpecializationInfo _spec_info{};
    VkVertexInputBindingDescription _binding_description{};
    std::vector<VkVertexInputAttributeDescription> _attribute_descriptions;
};

#endif // SHADER_H_
