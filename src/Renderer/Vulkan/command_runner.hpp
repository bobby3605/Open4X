#ifndef COMMAND_RUNNER_H_
#define COMMAND_RUNNER_H_
#include <vector>
#include <vulkan/vulkan_core.h>

class CommandRunner {
  public:
    CommandRunner();
    void run();
    void copy_buffer(VkBuffer dst, VkBuffer src, std::vector<VkBufferCopy>& copy_infos);

  private:
    static VkCommandPool _pool;
    VkCommandBuffer _command_buffer;
    void begin_recording();
    void end_recording();
};

#endif // COMMAND_RUNNER_H_
