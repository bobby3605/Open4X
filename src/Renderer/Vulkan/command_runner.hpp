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
    static std::pair<std::shared_ptr<VkImageMemoryBarrier2>, std::shared_ptr<VkDependencyInfo>>
    transition_image(VkImageLayout old_layout, VkImageLayout new_layout, uint32_t mip_levels);
    static std::pair<std::shared_ptr<VkImageMemoryBarrier2>, std::shared_ptr<VkDependencyInfo>>
    transition_image(VkImageLayout old_layout, VkImageLayout new_layout, VkImageSubresourceRange subresource_range);
    static std::pair<std::shared_ptr<VkImageMemoryBarrier2>, std::shared_ptr<VkDependencyInfo>>
    image_barrier(VkImageMemoryBarrier2& barrier);

  private:
    static VkCommandPool _pool;
    VkCommandBuffer _command_buffer;
    void begin_recording();
    void end_recording();
};

#endif // COMMAND_RUNNER_H_
