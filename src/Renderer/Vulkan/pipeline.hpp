#ifndef PIPELINE_H_
#define PIPELINE_H_
#include "descriptors.hpp"
#include "shader.hpp"
#include <string>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

class Pipeline {
  public:
    Pipeline();
    Pipeline(LinearAllocator<GPUAllocation>* descriptor_buffer_allocator);
    ~Pipeline();
    VkPipeline const& vk_pipeline() { return _pipeline; }
    VkPipelineLayout const& vk_pipeline_layout() { return _pipeline_layout; }
    VkPipelineBindPoint const& bind_point() { return _bind_point; }
    std::string const& name() { return _pipeline_name; }
    std::vector<Shader*> const& shaders() const { return _shaders; }
    DescriptorLayout const& descriptor_layout() const { return _descriptor_layout; }
    void update_descriptors();

  protected:
    VkPipeline _pipeline;
    VkPipelineLayout _pipeline_layout;
    VkPipelineBindPoint _bind_point;
    std::string _pipeline_name;
    DescriptorLayout _descriptor_layout;
    void load_shaders(std::vector<std::filesystem::path> const& shader_paths, std::vector<void*> const& specialization_data);
    std::vector<Shader*> _shaders;
    std::vector<VkPipelineShaderStageCreateInfo> _shader_stages;
};

class GraphicsPipeline : public Pipeline {
  public:
    GraphicsPipeline(VkPipelineRenderingCreateInfo& pipeline_rendering_info, VkExtent2D extent, std::filesystem::path const& vert_path,
                     std::filesystem::path const& frag_path, void* vert_spec_data, void* frag_spec_data,
                     VkPrimitiveTopology const& topology);
    GraphicsPipeline(VkPipelineRenderingCreateInfo& pipeline_rendering_info, VkExtent2D extent, std::filesystem::path const& vert_path,
                     std::filesystem::path const& frag_path, void* vert_spec_data, void* frag_spec_data,
                     VkPrimitiveTopology const& topology, LinearAllocator<GPUAllocation>* descriptor_buffer_allocator);
    ~GraphicsPipeline();

  private:
    void create(VkPipelineRenderingCreateInfo& pipeline_rendering_info, VkExtent2D extent, std::filesystem::path const& vert_path,
                std::filesystem::path const& frag_path, void* vert_spec_data, void* frag_spec_data, VkPrimitiveTopology const& topology);
};
class ComputePipeline : public Pipeline {
  public:
    ComputePipeline(std::filesystem::path const& compute_path, void* compute_spec_data);
    ComputePipeline(std::filesystem::path const& compute_path, void* compute_spec_data,
                    LinearAllocator<GPUAllocation>* descriptor_buffer_allocator);

  private:
    void create(std::filesystem::path const& compute_path, void* compute_spec_data);
};

#endif // PIPELINE_H_
