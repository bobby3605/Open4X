#ifndef SWAPCHAIN_H_
#define SWAPCHAIN_H_

#include <vector>
#include <vulkan/vulkan_core.h>

class SwapChain {
  public:
    SwapChain(VkExtent2D _extent);
    ~SwapChain();

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
    VkImage _depth_image;
    VkDeviceMemory _depth_image_memory;
    VkImageView _depth_image_view;
    void create_depth_resources();
    std::vector<VkSemaphore> _image_available_semaphores;
    std::vector<VkSemaphore> _render_finished_semaphores;
    std::vector<VkFence> _in_flight_fences;
    void create_sync_objects();
};

#endif // SWAPCHAIN_H_
