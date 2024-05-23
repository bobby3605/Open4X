#ifndef PIPELINE_H_
#define PIPELINE_H_
#include "shader.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

class Pipeline {
  public:
    ~Pipeline();
    VkPipeline const& vk_pipeline() { return _pipeline; }
    VkPipelineBindPoint const& bind_point() { return _bind_point; }
    VkDescriptorBufferBindingInfoEXT const& descriptor_buffers_binding_info() { return _descriptor_buffers_binding_info; }
    static constexpr std::string_view _descriptor_buffer_suffix = "_descriptor_buffer";
    std::string const& name() { return _pipeline_name; }
    std::unordered_map<std::string, Shader> const& shaders() { return _shaders; }

  protected:
    VkPipeline _pipeline;
    VkPipelineLayout _pipeline_layout;
    VkPipelineBindPoint _bind_point;
    std::string _pipeline_name;
    VkDescriptorBufferBindingInfoEXT _descriptor_buffers_binding_info{VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT};
    std::unordered_map<std::string, Shader> _shaders;
};

class GraphicsPipeline : public Pipeline {
  public:
    GraphicsPipeline(VkPipelineRenderingCreateInfo& pipeline_rendering_info, VkExtent2D extent, std::string const& vert_path,
                     std::string const& frag_path);
    ~GraphicsPipeline();
};

#endif // PIPELINE_H_
