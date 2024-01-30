#ifndef VULKAN_RENDERGRAPH_H_
#define VULKAN_RENDERGRAPH_H_
#include "vulkan_buffer.hpp"
#include "vulkan_descriptors.hpp"
#include "vulkan_pipeline.hpp"
#include "vulkan_swapchain.hpp"
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vulkan/vulkan_core.h>

class VulkanRenderGraph {
  public:
    typedef std::unordered_map<std::string, std::shared_ptr<VulkanBuffer>> bufferMap;
    typedef std::unordered_map<std::string, uint32_t> bufferCountMap;
    typedef std::function<void(VkCommandBuffer)> RenderOp;
    VulkanRenderGraph(std::shared_ptr<VulkanDevice> device);
    class VulkanShader {
      public:
        VulkanShader(std::string path, VkSpecializationInfo* specInfo);
        ~VulkanShader();

      protected:
        std::string path;
        std::string name;
        std::shared_ptr<VulkanDevice> _device;
        VkShaderStageFlagBits stageFlags;
        VkPipelineBindPoint bindPoint;

        std::string _preamble = "";
        std::string _entryPoint = "main";
        void compile();
        std::vector<glslang::TObjectReflection> buffers;
        std::vector<uint32_t> spirv;

        std::vector<uint32_t> code;
        void createShaderModule();
        VkShaderModule shaderModule;
        VkPipelineShaderStageCreateInfo stageInfo{};

        VkPushConstantRange pushConstantRange{};
        void setDescriptorBuffers(VulkanDescriptors::VulkanDescriptor* descriptor, bufferCountMap& bufferCounts, bufferMap& globalBuffers);
        friend class VulkanRenderGraph;
    };

    VulkanRenderGraph& shader(std::string path, VkSpecializationInfo* specInfo = VK_NULL_HANDLE);
    VulkanRenderGraph& buffer(std::string name, uint32_t count);
    VulkanRenderGraph& buffer(std::string name, std::shared_ptr<VulkanBuffer> buffer);
    void compile();

  private:
    std::vector<VkCommandBuffer> commandBuffers;
    std::shared_ptr<VulkanSwapChain> swapChain;
    size_t getCurrentFrame() const { return swapChain->currentFrame(); }
    VkCommandBuffer getCurrentCommandBuffer() { return commandBuffers[getCurrentFrame()]; }
    void addComputePipeline(std::shared_ptr<VulkanShader> computeShader, std::vector<VkDescriptorSetLayout>& layouts);
    void addGraphicsPipeline(std::shared_ptr<VulkanShader> vertShader, std::shared_ptr<VulkanShader> fragShader);
    std::vector<VkPushConstantRange> getPushConstants(std::shared_ptr<VulkanRenderGraph::VulkanShader> shader);

    std::shared_ptr<VulkanDescriptors> descriptorManager;
    std::shared_ptr<VulkanDevice> _device;
    std::unordered_map<std::string, std::shared_ptr<VulkanPipeline>> pipelines;
    //    std::vector<std::shared_ptr<GraphicsPipeline>> graphicsPipelines;
    bufferMap globalBuffers;
    bufferCountMap bufferCounts;
    std::vector<std::shared_ptr<VulkanShader>> shaders;

    // Adds support for multiple push constant layouts in a single pipeline
    // For example, if you want different push constants for vertex and fragment shader
    uint32_t pushConstantOffset = 0;

    std::vector<RenderOp> renderOps;
    void recordCommandBuffer(VkCommandBuffer commandBuffer);
    RenderOp bindPipeline(std::shared_ptr<VulkanRenderGraph::VulkanShader> shader);
    RenderOp bindDescriptorSets(std::shared_ptr<VulkanShader> shader);
    RenderOp pushConstants(std::shared_ptr<VulkanShader> shader, uint32_t size, void* data, uint32_t offset = 0);
};

#endif // VULKAN_RENDERGRAPH_H_
