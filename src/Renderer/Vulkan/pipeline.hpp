#ifndef PIPELINE_H_
#define PIPELINE_H_
#include "descriptors.hpp"
#include "shader.hpp"
#include <string>
#include <unordered_map>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

class Pipeline {
  public:
    Pipeline();
    Pipeline(LinearAllocator<GPUAllocator>* descriptor_buffer_allocator);
    ~Pipeline();
    VkPipeline const& vk_pipeline() { return _pipeline; }
    VkPipelineLayout const& vk_pipeline_layout() { return _pipeline_layout; }
    VkPipelineBindPoint const& bind_point() { return _bind_point; }
    std::string const& name() { return _pipeline_name; }
    std::unordered_map<std::string, Shader> const& shaders() const { return _shaders; }
    DescriptorLayout const& descriptor_layout() const { return _descriptor_layout; }
    void update_descriptors();

  protected:
    VkPipeline _pipeline;
    VkPipelineLayout _pipeline_layout;
    VkPipelineBindPoint _bind_point;
    std::string _pipeline_name;
    DescriptorLayout _descriptor_layout;
    std::unordered_map<std::string, Shader> _shaders;
};

class GraphicsPipeline : public Pipeline {
  public:
    GraphicsPipeline(VkPipelineRenderingCreateInfo& pipeline_rendering_info, VkExtent2D extent, std::string const& vert_path,
                     std::string const& frag_path);
    GraphicsPipeline(VkPipelineRenderingCreateInfo& pipeline_rendering_info, VkExtent2D extent, std::string const& vert_path,
                     std::string const& frag_path, LinearAllocator<GPUAllocator>* descriptor_buffer_allocator);
    ~GraphicsPipeline();

  private:
    void create(VkPipelineRenderingCreateInfo& pipeline_rendering_info, VkExtent2D extent, std::string const& vert_path,
                std::string const& frag_path);
};

#endif // PIPELINE_H_
