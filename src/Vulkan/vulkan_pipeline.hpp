#ifndef VULKAN_PIPELINE_H_
#define VULKAN_PIPELINE_H_

#include "vulkan_descriptors.hpp"
#include "vulkan_device.hpp"
#include "vulkan_swapchain.hpp"
#include <cstdint>
#include <glslang/MachineIndependent/Versions.h>
#include <glslang/MachineIndependent/localintermediate.h>
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

class VulkanPipeline {
  public:
    //    VulkanPipeline(std::shared_ptr<VulkanDevice> device, VkGraphicsPipelineCreateInfo pipelineInfo);
    //   VulkanPipeline(std::shared_ptr<VulkanDevice> device, std::string computeShaderPath, std::vector<VkDescriptorSetLayout>&
    //   descriptorLayouts,
    //                 std::vector<VkPushConstantRange>& pushConstants, VkSpecializationInfo* specializationInfo);

    VulkanPipeline(std::shared_ptr<VulkanDevice> device) : _device{device} {}

    ~VulkanPipeline();
    static VkGraphicsPipelineCreateInfo defaultPipelineConfigInfo();
    VkPipeline pipeline() { return _pipeline; }
    VkPipelineLayout pipelineLayout() { return _pipelineLayout; }
    //    virtual void bind();

  protected:
    void createGraphicsPipeline();
    void createComputePipeline();
    VkShaderModule createShaderModule(const std::vector<uint32_t>& code);
    std::vector<uint32_t> compileShader(std::string filePath);

    void createPipelineLayout(std::vector<VkDescriptorSetLayout>& descriptorLayouts, std::vector<VkPushConstantRange>& pushConstants);

    std::string _entryPoint;

    std::shared_ptr<VulkanDevice> _device;
    VkPipeline _pipeline;
    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
    VkPipelineBindPoint _pipelineType;
};

class GraphicsPipeline : public VulkanPipeline {
  public:
    GraphicsPipeline(std::shared_ptr<VulkanDevice> device, VulkanSwapChain* swapChain, VkPipelineShaderStageCreateInfo vertInfo,
                     VkPipelineShaderStageCreateInfo fragInfo, std::vector<VkDescriptorSetLayout>& descriptorLayouts,
                     std::vector<VkPushConstantRange>& pushConstants, VkPrimitiveTopology topology);
    //   void bind();
};

class ComputePipeline : public VulkanPipeline {
  public:
    ComputePipeline(std::shared_ptr<VulkanDevice> device, std::vector<VkDescriptorSetLayout>& descriptorLayouts,
                    std::vector<VkPushConstantRange>& pushConstants, VkPipelineShaderStageCreateInfo stageInfo);
    //  void bind();
};

#endif // VULKAN_PIPELINE_H_
