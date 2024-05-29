#ifndef RENDEROPS_H_
#define RENDEROPS_H_

#include "../../utils.hpp"
#include "buffer.hpp"
#include "pipeline.hpp"
#include "swapchain.hpp"
#include <cstdint>
#include <functional>
#include <future>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vulkan/vulkan_core.h>

typedef std::function<void(VkCommandBuffer)> RenderOp;
// Needs to be shared and not unique for type erasure
typedef std::shared_ptr<void> RenderDep;
typedef std::vector<RenderDep> RenderDeps;

enum HazardType { READ, WRITE, READ_WRITE };

struct Hazard {
    std::string buffer_name;
    HazardType type;
};

struct ReadHazard : Hazard {};
struct WriteHazard : Hazard {};

class RenderNode {
  public:
    template <typename F, typename... Args> RenderNode(RenderDeps in_deps, F f, Args... args) : op{partial(f, args...)}, deps{in_deps} {}
    RenderOp op;
    std::vector<Hazard> hazards;
    // NOTE:
    // Keep track of any data dependencies
    // Example:
    // vkCmdPipelineBarrier2 takes a pointer to VkDependencyInfo
    // It can't be stack allocated, because the VkDependencyInfo will be deleted by the time the node is recorded
    // Keeping track of it as a shared pointer allows the data to persist for the lifetime of the RenderNode
    RenderDeps deps;
};

class RenderGraph {
  public:
    RenderGraph(VkCommandPool pool);
    ~RenderGraph();
    void compile();
    VkResult submit(SwapChain* swap_chain);
    // TODO
    // Clean this and RenderNode up,
    // shouldn't be templating it twice,
    // really 3 times because of the partial template
    template <typename F, typename... Args> void add_node(RenderDeps deps, F f, Args... args) {
        if (_secondary) {
            _secondary_graphs.try_emplace(_curr_secondary_cmd_buffer).first->second.emplace_back(deps, f, args...);
        } else {
            _primary_graphs.try_emplace(_curr_primary_cmd_buffer).first->second.emplace_back(deps, f, args...);
        }
    }
    template <typename F, typename... Args> void add_node(F f, Args... args) { add_node({}, f, args...); }

    void transition_image(VkImage image, VkImageLayout old_layout, VkImageLayout new_layout, uint32_t mip_levels);
    void transition_image(VkImage image, VkImageLayout old_layout, VkImageLayout new_layout, VkImageSubresourceRange subresource_range);
    void begin_rendering(SwapChain* const swap_chain);
    void end_rendering(SwapChain* const swap_chain);
    // TODO
    // Remove the SwapChain dependency
    void graphics_pass(std::string const& vert_path, std::string const& frag_path, LinearAllocator<GPUAllocator>* vertex_buffer_allocator,
                       LinearAllocator<GPUAllocator>* index_buffer_allocator, SwapChain* swap_chain_defaults);
    void buffer(std::string name, VkDeviceSize size);
    void define_primary(bool per_frame);
    void define_secondary(bool per_frame);

  private:
    VkCommandPool _pool;
    bool _secondary = false;
    VkCommandBuffer _curr_primary_cmd_buffer;
    VkCommandBuffer _curr_secondary_cmd_buffer;
    std::unordered_map<VkCommandBuffer, std::vector<RenderNode>> _primary_graphs;
    std::vector<VkCommandBuffer> _per_frame_primary;
    std::unordered_map<VkCommandBuffer, std::vector<RenderNode>> _secondary_graphs;
    std::vector<VkCommandBuffer> _per_frame_secondary;
    void record_buffer(VkCommandBuffer command_buffer, VkCommandBufferUsageFlags flags, std::vector<RenderNode> const& render_nodes);
    void image_barrier(VkImageMemoryBarrier2& barrier);
    std::vector<std::shared_ptr<Pipeline>> _pipelines;
    std::unordered_map<std::string, VkDeviceSize> _buffer_size_registry;
    LinearAllocator<GPUAllocator>* _descriptor_buffer_allocator;

    void bad_workaround(SwapChain* swap_chain);
};

#endif // RENDEROPS_H_
