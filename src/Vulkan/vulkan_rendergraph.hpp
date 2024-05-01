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
        VkPrimitiveTopology topology;

        void setDescriptorBuffers(VulkanDescriptors::VulkanDescriptor* descriptor, bufferCreateInfoMap& bufferCounts,
                                  bufferMap& globalBuffers, imageInfosMap& globalImageInfos);
        friend class VulkanRenderGraph;
    };

    VulkanRenderGraph& shader(std::string computePath, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ,
                              ShaderOptions shaderOptions);
    template <typename VT, typename IT>
    VulkanRenderGraph& shader(std::string vertPath, std::string fragPath, ShaderOptions vertOptions, ShaderOptions fragOptions,
                              std::vector<VT> vertices, std::vector<IT> indices) {
        std::shared_ptr<VulkanRenderGraph::VulkanShader> vert = std::make_shared<VulkanRenderGraph::VulkanShader>(vertPath, _device);
        std::shared_ptr<VulkanRenderGraph::VulkanShader> frag = std::make_shared<VulkanRenderGraph::VulkanShader>(fragPath, _device);
        shaders.push_back(vert);
        shaders.push_back(frag);
        //        renderOps.push_back(startRendering());

        renderOps.push_back(bindPipeline(vert));
        // FIXME:
        // Find better way to name descriptor sets,
        // possibly by pipeline name
        renderOps.push_back(bindDescriptorSets(vert));
        renderOps.push_back(bindDescriptorSets(frag));
        ShaderOptions defaultOptions{};
        if (vertOptions.pushConstantData != defaultOptions.pushConstantData) {
            // TODO
            // Only push constants if the data pointer has changed between shaders
            renderOps.push_back(pushConstants(vert, vertOptions.pushConstantData));
        }
        std::string pipeline_name = getFilenameNoExt(vert->name);

        // TODO:
        // Better way of doing this
        if (pipeline_name == "triangle") {
            vert->topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        } else if (pipeline_name == "box") {
            vert->topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        }

        buffer(pipeline_name + "_vertex_buffer", vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        buffer(pipeline_name + "_index_buffer", indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

        renderOps.push_back(bindVertexBuffer(pipeline_name + "_vertex_buffer"));
        if (fragOptions.pushConstantData != defaultOptions.pushConstantData) {
            // TODO
            // Only push constants if the data pointer has changed between shaders
            renderOps.push_back(pushConstants(frag, fragOptions.pushConstantData));
        }
        if (vert->hasSpecConstants) {
            vert->specInfo.pData = vertOptions.specData;
        }
        if (frag->hasSpecConstants) {
            frag->specInfo.pData = fragOptions.specData;
        }
        renderOps.push_back(bindIndexBuffer(pipeline_name + "_index_buffer"));
        return *this;
    }
    VulkanRenderGraph& buffer(std::string name, uint32_t count);
    VulkanRenderGraph& buffer(std::string name, uint32_t count, VkBufferUsageFlags additionalUsage,
                              VkMemoryPropertyFlags additionalProperties);
    VulkanRenderGraph& buffer(std::string name, std::shared_ptr<VulkanBuffer> buffer);

    template <typename T> VulkanRenderGraph& buffer(std::string name, std::vector<T> data, VkBufferUsageFlags additionalUsage) {
        buffer(name, VulkanBuffer::StagedBuffer(_device, (void*)data.data(), sizeof(T) * data.size(), additionalUsage));
        return *this;
    }
    VulkanRenderGraph& imageInfos(std::string name, std::vector<VkDescriptorImageInfo>* imageInfos);
    VulkanRenderGraph& fillBuffer(std::string name, VkDeviceSize offset, VkDeviceSize size, uint32_t value);
    VulkanRenderGraph& setBuffer(std::string name, uint32_t value);
    VulkanRenderGraph& drawIndirect(std::string buffer, VkDeviceSize offset, std::string countBuffer, VkDeviceSize countBufferOffset,
                                    uint32_t maxDrawCount, uint32_t stride);
    VulkanRenderGraph& drawIndirect(std::string buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride);
    VulkanRenderGraph& timestamp(VkPipelineStageFlags2 stageFlags, VkQueryPool queryPool, uint32_t query);
    VulkanRenderGraph& queryReset(VkQueryPool queryPool, uint32_t firstQuery, uint32_t count);
    VulkanRenderGraph& memoryBarrier(VkAccessFlags2 srcAccessMask, VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 dstAccessMask,
                                     VkPipelineStageFlags2 dstStageMask);
    VulkanRenderGraph& debugBarrier();
    VulkanRenderGraph& resetViewport(VkViewport viewport);
    VulkanRenderGraph& resetScissor(VkRect2D scissor);

    VulkanRenderGraph& startRendering();
    VulkanRenderGraph& endRendering();
    VulkanRenderGraph& outputTransition();

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
    RenderOp drawIndexedIndirect(std::string bufferName, VkDeviceSize offset, uint32_t drawCount, uint32_t stride);

    RenderOp startRenderingOp();
    RenderOp endRenderingOp();
    RenderOp outputTransitionOp();
    RenderOp writeTimestamp(VkPipelineStageFlags2 stageFlags, VkQueryPool queryPool, uint32_t query);
    RenderOp resetQueryPool(VkQueryPool queryPool, uint32_t firstQuery, uint32_t count);
    RenderOp bindVertexBuffer(std::string vertex_buffer);
    RenderOp bindIndexBuffer(std::string index_buffer);

    RenderOp memoryBarrierOp(VkAccessFlags2 srcAccessMask, VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 dstAccessMask,
                             VkPipelineStageFlags2 dstStageMask);
    RenderOp setViewportOp(VkViewport viewport);
    RenderOp setScissorOp(VkRect2D scissor);
};

#endif // VULKAN_RENDERGRAPH_H_
