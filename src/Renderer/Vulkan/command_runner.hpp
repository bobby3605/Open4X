#ifndef COMMAND_RUNNER_H_
#define COMMAND_RUNNER_H_
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

class CommandRunner {
  public:
    CommandRunner();
    void run();
    void copy_buffer(VkBuffer dst, VkBuffer src, std::vector<VkBufferCopy>& copy_infos);
    void copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    static std::pair<std::shared_ptr<VkImageMemoryBarrier2>, std::shared_ptr<VkDependencyInfo>>
    transition_image(VkImageLayout old_layout, VkImageLayout new_layout, uint32_t mip_levels);

    static std::pair<std::shared_ptr<VkImageMemoryBarrier2>, std::shared_ptr<VkDependencyInfo>>
    transition_image(VkImageLayout old_layout, VkImageLayout new_layout, VkImageSubresourceRange subresource_range);

    void transition_image(VkImageLayout old_layout, VkImageLayout new_layout, uint32_t mip_levels, VkImage image);

    void transition_image(VkImageLayout old_layout, VkImageLayout new_layout, VkImageSubresourceRange subresource_range, VkImage image);

    static std::pair<std::shared_ptr<VkImageMemoryBarrier2>, std::shared_ptr<VkDependencyInfo>>
    image_barrier(VkImageMemoryBarrier2& barrier);

  private:
    static VkCommandPool _pool;
    VkCommandBuffer _command_buffer;
    void begin_recording();
    void end_recording();
    std::vector<std::shared_ptr<void>> _dependencies;
};

#endif // COMMAND_RUNNER_H_
