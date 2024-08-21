#ifndef RENDEROPS_H_
#define RENDEROPS_H_

#include "../../utils/utils.hpp"
#include "../Allocator/allocation.hpp"
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
    void graphics_pass(std::string const& vert_path, std::string const& frag_path, LinearAllocator<GPUAllocation>* vertex_buffer_allocator,
                       LinearAllocator<GPUAllocation>* index_buffer_allocator);
    void buffer(std::string name, VkDeviceSize size);
    template <typename T> void set_push_constant(std::string const& name, T data) {
        *reinterpret_cast<T*>(_push_constants[name].get()) = data;
    };

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
};

#endif // RENDEROPS_H_
