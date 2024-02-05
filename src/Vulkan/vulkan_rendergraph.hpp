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
#include <vector>
#include <vulkan/vulkan_core.h>

class VulkanRenderGraph {
  public:
    typedef std::unordered_map<std::string, std::shared_ptr<VulkanBuffer>> bufferMap;
    typedef std::unordered_map<std::string, std::vector<VkDescriptorImageInfo>*> imageInfosMap;
    typedef std::unordered_map<std::string, std::tuple<uint32_t, VkBufferUsageFlags, VkMemoryPropertyFlags>> bufferCreateInfoMap;
    struct ShaderOptions {
        void* pushConstantData = VK_NULL_HANDLE;
        void* specData = VK_NULL_HANDLE;
    };
    VulkanRenderGraph(std::shared_ptr<VulkanDevice> device, VulkanWindow* window, std::shared_ptr<Settings> settings);
    class VulkanShader {
      public:
        VulkanShader(std::string path, std::shared_ptr<VulkanDevice> device);
        ~VulkanShader();

      protected:
        std::string path;
        const std::string basePath = "assets/shaders/";
        std::string name;
        std::shared_ptr<VulkanDevice> _device;
        VkShaderStageFlagBits stageFlags;
        VkPipelineBindPoint bindPoint;

        std::string _preamble = "";
        std::string _entryPoint = "main";
        void compile();
        std::vector<glslang::TObjectReflection> buffers;
        std::vector<uint32_t> spirv;

        void createShaderModule();
        VkShaderModule shaderModule = VK_NULL_HANDLE;
        VkPipelineShaderStageCreateInfo stageInfo{};

        VkPushConstantRange pushConstantRange{};
        bool hasPushConstants = false;
        VkSpecializationInfo specInfo{};
        std::vector<VkSpecializationMapEntry> specEntries;
        bool hasSpecConstants = false;

        void setDescriptorBuffers(VulkanDescriptors::VulkanDescriptor* descriptor, bufferCreateInfoMap& bufferCounts,
                                  bufferMap& globalBuffers, imageInfosMap& globalImageInfos);
        friend class VulkanRenderGraph;
    };

    VulkanRenderGraph& shader(std::string computePath, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ,
                              ShaderOptions shaderOptions);
    VulkanRenderGraph& shader(std::string vertPath, std::string fragPath, ShaderOptions vertOptions, ShaderOptions fragOptions,
                              std::shared_ptr<VulkanBuffer> vertexBuffer, std::shared_ptr<VulkanBuffer> indexBuffer);
    VulkanRenderGraph& buffer(std::string name, uint32_t count);
    VulkanRenderGraph& buffer(std::string name, uint32_t count, VkBufferUsageFlags additionalUsage,
                              VkMemoryPropertyFlags additionalProperties);
    VulkanRenderGraph& buffer(std::string name, std::shared_ptr<VulkanBuffer> buffer);

    template <typename T> VulkanRenderGraph& buffer(std::string name, std::vector<T> data, VkBufferUsageFlags additionalUsage) {
        buffer(name, VulkanBuffer::StagedBuffer(_device, (void*)data.data(), sizeof(data[0]) * data.size(), additionalUsage));
        return *this;
    }
    VulkanRenderGraph& imageInfos(std::string name, std::vector<VkDescriptorImageInfo>* imageInfos);
    VulkanRenderGraph& fillBuffer(std::string name, VkDeviceSize offset, VkDeviceSize size, uint32_t value);
    VulkanRenderGraph& setBuffer(std::string name, uint32_t value);
    VulkanRenderGraph& drawIndirect(std::string buffer, VkDeviceSize offset, std::string countBuffer, VkDeviceSize countBufferOffset,
                                    uint32_t maxDrawCount, uint32_t stride);
    VulkanRenderGraph& timestamp(VkPipelineStageFlags2 stageFlags, VkQueryPool queryPool, uint32_t query);
    VulkanRenderGraph& queryReset(VkQueryPool queryPool, uint32_t firstQuery, uint32_t count);
    VulkanRenderGraph& memoryBarrier(VkAccessFlags2 srcAccessMask, VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 dstAccessMask,
                                     VkPipelineStageFlags2 dstStageMask);
    VulkanRenderGraph& debugBarrier();
    VulkanRenderGraph& resetViewport(VkViewport viewport);
    VulkanRenderGraph& resetScissor(VkRect2D scissor);
    void compile();
    void bufferWrite(std::string name, void* data);
    VkExtent2D getSwapChainExtent() { return swapChain->getExtent(); }
    bool render();

  private:
    std::vector<VkCommandBuffer> commandBuffers;
    VulkanSwapChain* swapChain;
    void createCommandBuffers();
    size_t getCurrentFrame() const { return swapChain->currentFrame(); }
    VkCommandBuffer getCurrentCommandBuffer() { return commandBuffers[getCurrentFrame()]; }
    void addComputePipeline(std::shared_ptr<VulkanShader> computeShader, std::vector<VkDescriptorSetLayout>& layouts);
    void addGraphicsPipeline(std::shared_ptr<VulkanShader> vertShader, std::shared_ptr<VulkanShader> fragShader);
    std::vector<VkPushConstantRange> getPushConstants(std::shared_ptr<VulkanRenderGraph::VulkanShader> shader);
    void resetViewScissor(VkCommandBuffer commandBuffer);

    std::shared_ptr<VulkanDescriptors> descriptorManager;
    std::shared_ptr<VulkanDevice> _device;
    VulkanWindow* _window;
    std::shared_ptr<Settings> _settings;
    std::unordered_map<std::string, std::shared_ptr<VulkanPipeline>> pipelines;
    //    std::vector<std::shared_ptr<GraphicsPipeline>> graphicsPipelines;
    bufferMap globalBuffers;
    bufferCreateInfoMap bufferCreateInfos;
    imageInfosMap globalImageInfos;
    std::vector<std::shared_ptr<VulkanShader>> shaders;

    // Adds support for multiple push constant layouts in a single pipeline
    // For example, if you want different push constants for vertex and fragment shader
    uint32_t pushConstantOffset = 0;

    void startFrame();
    bool endFrame();

    void recreateSwapChain();

    std::vector<RenderOp> renderOps;
    void recordRenderOps(VkCommandBuffer commandBuffer);
    RenderOp bindPipeline(std::shared_ptr<VulkanRenderGraph::VulkanShader> shader);
    RenderOp bindDescriptorSets(std::shared_ptr<VulkanShader> shader);
    RenderOp pushConstants(std::shared_ptr<VulkanShader> shader, void* data);
    RenderOp fillBufferOp(std::string bufferName, VkDeviceSize offset, VkDeviceSize size, uint32_t value);
    RenderOp dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
    RenderOp drawIndexedIndirectCount(std::string bufferName, VkDeviceSize offset, std::string countBufferName,
                                      VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride);
    RenderOp startRendering();
    RenderOp endRendering();
    RenderOp writeTimestamp(VkPipelineStageFlags2 stageFlags, VkQueryPool queryPool, uint32_t query);
    RenderOp resetQueryPool(VkQueryPool queryPool, uint32_t firstQuery, uint32_t count);
    RenderOp bindVertexBuffer(std::shared_ptr<VulkanBuffer> vertexBuffer);
    RenderOp bindIndexBuffer(std::shared_ptr<VulkanBuffer> indexBuffer);

    RenderOp memoryBarrierOp(VkAccessFlags2 srcAccessMask, VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 dstAccessMask,
                             VkPipelineStageFlags2 dstStageMask);
    RenderOp setViewportOp(VkViewport viewport);
    RenderOp setScissorOp(VkRect2D scissor);
};

#endif // VULKAN_RENDERGRAPH_H_
