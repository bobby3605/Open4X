#ifndef SWAPCHAIN_H_
#define SWAPCHAIN_H_

#include <vector>
#include <vulkan/vulkan_core.h>

class SwapChain {
  public:
    SwapChain(VkExtent2D _extent);
    ~SwapChain();
    size_t current_frame() const { return _current_frame; }
    VkResult acquire_next_image();
    VkResult submit_command_buffer(VkCommandBuffer& cmd_buffer);
    VkSurfaceFormatKHR const& surface_format() { return _surface_format; }
    VkFormat const& depth_format() { return _depth_format; }
    VkImageView color_image_view() { return _color_image_view; }
    VkImageView depth_image_view() { return _depth_image_view; }
    VkImage const& color_image() { return _color_image; }
    VkImage const& depth_image() { return _depth_image; }
    VkExtent2D extent() { return _extent; }
    VkImage current_image() const { return _swap_chain_images[_image_index]; }
    VkImageView current_image_view() const { return _swap_chain_image_views[current_frame()]; }

    static const uint32_t MAX_FRAMES_IN_FLIGHT = 1;

  private:
    VkSwapchainKHR _swap_chain;
    VkSurfaceFormatKHR _surface_format;
    void choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats);
    VkExtent2D _extent;
    void clamp_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities);
    std::vector<VkImage> _swap_chain_images;
    void create_swap_chain(VkSwapchainKHR _old_swap_chain = VK_NULL_HANDLE);
    std::vector<VkImageView> _swap_chain_image_views;
    void create_image_views();
    VkImage _color_image;
    VkDeviceMemory _color_image_memory;
    VkImageView _color_image_view;
    void create_color_resources();
    VkFormat find_supported_format(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat find_depth_format();
    VkFormat _depth_format;
    VkImage _depth_image;
    VkDeviceMemory _depth_image_memory;
    VkImageView _depth_image_view;
    void create_depth_resources();
    std::vector<VkSemaphore> _image_available_semaphores;
    std::vector<VkSemaphore> _render_finished_semaphores;
    std::vector<VkFence> _in_flight_fences;
    void create_sync_objects();
    size_t _current_frame = 0;
    uint32_t _image_index = 0;
};

#endif // SWAPCHAIN_H_
