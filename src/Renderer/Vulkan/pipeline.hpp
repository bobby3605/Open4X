#ifndef PIPELINE_H_
#define PIPELINE_H_
#include "shader.hpp"
#include <string>
#include <unordered_map>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

class Pipeline {
  public:
    ~Pipeline();
    VkPipeline const& vk_pipeline() { return _pipeline; }
    VkPipelineBindPoint const& bind_point() { return _bind_point; }
    std::string const& name() { return _pipeline_name; }
    std::unordered_map<std::string, Shader> const& shaders() const { return _shaders; }

  protected:
    VkPipeline _pipeline;
    VkPipelineLayout _pipeline_layout;
    VkPipelineBindPoint _bind_point;
    std::string _pipeline_name;
    std::unordered_map<std::string, Shader> _shaders;
};

class GraphicsPipeline : public Pipeline {
  public:
    GraphicsPipeline(VkPipelineRenderingCreateInfo& pipeline_rendering_info, VkExtent2D extent, std::string const& vert_path,
                     std::string const& frag_path);
    ~GraphicsPipeline();
};

#endif // PIPELINE_H_
