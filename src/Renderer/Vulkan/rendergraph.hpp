#ifndef RENDEROPS_H_
#define RENDEROPS_H_

#include "../../utils.hpp"
#include "buffer.hpp"
#include "pipeline.hpp"
#include "swapchain.hpp"
#include <functional>
#include <future>
#include <iterator>
#include <memory>
#include <string>
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
    template <typename F, typename... Args> RenderNode(F f, Args... args) : op{partial(f, args...)} {}
    template <typename F, typename... Args> RenderNode(RenderDeps in_deps, F f, Args... args) : op{partial(f, args...)}, deps{in_deps} {}
    // op should never change once it's initialized
    const RenderOp op;
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
    void compile();
    VkCommandBuffer command_buffer() { return _command_buffer; }
    // TODO
    // Clean this and RenderNode up,
    // shouldn't be templating it twice,
    // really 3 times because of the partial template
    template <typename F, typename... Args> void add_node(F f, Args... args) { _render_nodes.emplace_back(f, args...); }
    template <typename F, typename... Args> void add_node(RenderDeps deps, F f, Args... args) {
        _render_nodes.emplace_back(deps, f, args...);
    }

    void transition_image(VkImage image, VkImageLayout old_layout, VkImageLayout new_layout, uint32_t mip_levels);
    void transition_image(VkImage image, VkImageLayout old_layout, VkImageLayout new_layout, VkImageSubresourceRange subresource_range);
    void begin_rendering(SwapChain* const swap_chain);
    // TODO
    // Remove the SwapChain dependency
    void graphics_pass(std::string const& vert_path, std::string const& frag_path, std::string const& vertex_buffer_name,
                       std::string const& index_buffer_name, SwapChain* swap_chain_defaults);

  private:
    void get_descriptors();
    void record();
    std::vector<RenderNode> _render_nodes;
    VkCommandBuffer _command_buffer;
    void image_barrier(VkImageMemoryBarrier2& barrier);
    std::vector<std::shared_ptr<Pipeline>> _pipelines;
};

#endif // RENDEROPS_H_
