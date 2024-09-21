#ifndef RENDEROPS_H_
#define RENDEROPS_H_

#include "../../utils/utils.hpp"
#include "../Allocator/allocation.hpp"
#include "common.hpp"
#include "descriptor_manager.hpp"
#include "pipeline.hpp"
#include "swapchain.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vulkan/vulkan_core.h>

typedef std::function<void(VkCommandBuffer)> RenderOp;
// Needs to be shared and not unique for type erasure
typedef std::shared_ptr<void> RenderDep;
typedef std::vector<RenderDep> RenderDeps;
#define void_update []() {}

enum HazardType { READ, WRITE, READ_WRITE };

struct Hazard {
    std::string buffer_name;
    HazardType type;
};

struct ReadHazard : Hazard {};
struct WriteHazard : Hazard {};

class RenderNode {
  public:
    template <typename F, typename... Args>
    RenderNode(RenderDeps in_deps, std::function<void()> update_func, F f, Args... args)
        : op{partial(f, args...)}, deps{in_deps}, update{update_func} {}
    RenderOp op;
    std::vector<Hazard> hazards;
    // NOTE:
    // Keep track of any data dependencies
    // Example:
    // vkCmdPipelineBarrier2 takes a pointer to VkDependencyInfo
    // It can't be stack allocated, because the VkDependencyInfo will be deleted by the time the node is recorded
    // Keeping track of it as a shared pointer allows the data to persist for the lifetime of the RenderNode
    RenderDeps deps;
    std::function<void()> update;
};

class RenderGraph {
  public:
    RenderGraph(VkCommandPool pool);
    ~RenderGraph();
    // NOTE:
    // return true if swapchain recreated
    bool render();
    // TODO
    // Clean this and RenderNode up,
    // shouldn't be templating it twice,
    // really 3 times because of the partial template
    template <typename F, typename... Args> void add_node(RenderDeps deps, std::function<void()> update_func, F f, Args... args) {
        _graph.emplace_back(deps, update_func, f, args...);
    }
    template <typename F, typename... Args> void add_node(F f, Args... args) { add_node({}, void_update, f, args...); }
    void add_dynamic_node(RenderDeps deps, std::function<void(RenderNode& node)> set_op_func) {
        std::function<void()> update_func = []() {};
        auto op = [](VkCommandBuffer c, int a) {};
        add_node(deps, update_func, op, 0);

        // TODO
        // Better solution for non-struct vkCmd that need an update_func
        size_t last_node_index = _graph.size() - 1;
        _graph.back().update = [&, last_node_index, set_op_func]() { set_op_func(_graph[last_node_index]); };
    }

    void begin_rendering();
    void end_rendering();
    // TODO
    // Remove the SwapChain dependency
    void graphics_pass(std::filesystem::path const& vert_path, std::filesystem::path const& frag_path,
                       LinearAllocator<GPUAllocation>* vertex_buffer_allocator, LinearAllocator<GPUAllocation>* index_buffer_allocator,
                       void* vert_spec_data = nullptr, void* frag_spec_data = nullptr,
                       VkPrimitiveTopology const& topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    void compute_pass(std::filesystem::path const& compute_path, void* compute_spec_data = nullptr);
    void buffer(std::string name, VkDeviceSize size);
    template <typename T> void set_push_constant(std::string const& name, T data) {
        *reinterpret_cast<T*>(_push_constants[name].get()) = data;
    };
    char* get_push_constant(std::string const& name) { return _push_constants[name].get(); };

    void memory_barrier(VkAccessFlags2 src_access_mask, VkPipelineStageFlags2 src_stage_mask, VkAccessFlags2 dst_access_mask,
                        VkPipelineStageFlags2 dst_stage_mask);

  private:
    VkCommandPool _pool;
    std::vector<VkCommandBuffer> _command_buffers;
    std::vector<RenderNode> _graph;
    void record_buffer(VkCommandBuffer command_buffer, VkCommandBufferUsageFlags flags, std::vector<RenderNode> const& render_nodes);
    void image_barrier(VkImageMemoryBarrier2& barrier);
    LinearAllocator<GPUAllocation>* _descriptor_buffer_allocator;
    SwapChain* _swap_chain;
    VkResult submit();
    void recreate_swap_chain();
    std::unordered_map<std::string, std::shared_ptr<char[]>> _push_constants;

    template <typename T>
        requires std::is_base_of_v<Pipeline, T>
    void add_pipeline(std::shared_ptr<T> pipeline) {
        if (!Device::device->use_descriptor_buffers()) {
            auto vk_set_layouts = std::make_shared<std::vector<VkDescriptorSet>>(pipeline->descriptor_layout().vk_sets());
            add_node({pipeline, vk_set_layouts}, void_update, vkCmdBindDescriptorSets, pipeline->bind_point(),
                     pipeline->vk_pipeline_layout(), 0, vk_set_layouts->size(), vk_set_layouts->data(), 0, nullptr);
        }

        add_node(
            {pipeline},
            [&, pipeline]() {
                // TODO
                // Only update needed sets
                // Example:
                // global set at 0
                // vertex and fragment at 1 and 2
                // So only 1 and 2 need to be updated
                DescriptorManager::descriptor_manager->update(pipeline);
            },
            vkCmdBindPipeline, pipeline->bind_point(), pipeline->vk_pipeline());

        if (Device::device->use_descriptor_buffers()) {
            // Set descriptor offsets
            auto descriptor_buffer_offsets = std::make_shared<VkSetDescriptorBufferOffsetsInfoEXT>();
            descriptor_buffer_offsets->sType = VK_STRUCTURE_TYPE_SET_DESCRIPTOR_BUFFER_OFFSETS_INFO_EXT;
            descriptor_buffer_offsets->stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            descriptor_buffer_offsets->layout = pipeline->vk_pipeline_layout();
            descriptor_buffer_offsets->firstSet = 0;
            auto set_offsets = std::make_shared<std::vector<VkDeviceSize>>(pipeline->descriptor_layout().set_offsets());
            descriptor_buffer_offsets->setCount = set_offsets->size();
            auto buffer_indices = std::make_shared<std::vector<uint32_t>>(descriptor_buffer_offsets->setCount, 0);
            descriptor_buffer_offsets->pOffsets = set_offsets->data();
            descriptor_buffer_offsets->pBufferIndices = buffer_indices->data();

            add_node({descriptor_buffer_offsets, set_offsets, buffer_indices}, void_update, vkCmdSetDescriptorBufferOffsets2EXT_,
                     descriptor_buffer_offsets.get());
        }

        // push constants
        for (Shader* const& shader : pipeline->shaders()) {
            if (shader->has_push_constants()) {
                // if the memory for the push constants isn't allocated,
                // allocate it
                if (_push_constants.count(shader->push_constants_name()) == 0) {
                    _push_constants[shader->push_constants_name()] = std::shared_ptr<char[]>(new char[shader->push_constant_range().size]);
                }
                // set the push constants from the shader and get the pointer from the global map
                add_node({}, void_update, vkCmdPushConstants, pipeline->vk_pipeline_layout(), shader->stage_info().stage,
                         shader->push_constant_range().offset, shader->push_constant_range().size,
                         _push_constants[shader->push_constants_name()].get());
            }
        }
    }
};

#endif // RENDEROPS_H_
