#ifndef SHADER_H_
#define SHADER_H_
#include "descriptors.hpp"
#include <filesystem>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

class Shader {
  public:
    Shader(std::filesystem::path file_path, DescriptorLayout* pipeline_descriptor_layout);
    ~Shader();
    VkPipelineBindPoint bind_point() const { return _bind_point; }
    VkPipelineShaderStageCreateInfo stage_info() const { return _stage_info; }

  private:
    std::filesystem::path _path;
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
};

#endif // SHADER_H_
