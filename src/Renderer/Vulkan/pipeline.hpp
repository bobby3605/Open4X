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
    static constexpr std::string_view _descriptor_buffer_suffix = "_descriptor_buffer";
    std::string const& name() { return _pipeline_name; }
    std::unordered_map<std::string, Shader> const& shaders() const { return _shaders; }
    size_t buffer_size() const { return _buffer_size; }
    VkDescriptorBufferBindingInfoEXT descriptor_buffers_binding_info{VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT};

  protected:
    VkPipeline _pipeline;
    VkPipelineLayout _pipeline_layout;
    VkPipelineBindPoint _bind_point;
    std::string _pipeline_name;
    std::unordered_map<std::string, Shader> _shaders;
    size_t _buffer_size = 0;
};

class GraphicsPipeline : public Pipeline {
  public:
    GraphicsPipeline(VkPipelineRenderingCreateInfo& pipeline_rendering_info, VkExtent2D extent, std::string const& vert_path,
                     std::string const& frag_path);
    ~GraphicsPipeline();
};

#endif // PIPELINE_H_
