#ifndef DEVICE_H_
#define DEVICE_H_

#include "vk_mem_alloc.h"
#include <array>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan_core.h>

class Device {
  public:
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool is_complete() { return graphicsFamily.has_value() && presentFamily.has_value(); }
    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };
    Device();
    ~Device();

    static Device* device;

    void set_required_features();
    VkDevice const& vk_device() const { return _device; }
    VkInstance instance() const { return _instance; }
    VkSurfaceKHR surface() const { return _surface; }
    SwapChainSupportDetails query_swap_chain_support() { return query_swap_chain_support(_physical_device); }
    QueueFamilyIndices find_queue_families() { return find_queue_families(_physical_device); }
    void set_debug_name(VkObjectType type, uint64_t handle, std::string name);
    VkSampleCountFlagBits msaa_samples() const { return _msaa_samples; }
    static const VkBool32 sample_shading = VK_FALSE;
    VkPhysicalDeviceDescriptorBufferPropertiesEXT const& descriptor_buffer_properties() { return _descriptor_buffer_properties; };
    VkPhysicalDevice physical_device() const { return _physical_device; }
    VkImageView create_image_view(std::string name, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
    static const uint32_t API_VERSION = VK_API_VERSION_1_3;
    VkQueue graphics_queue() { return _graphics_queue; }
    VkQueue present_queue() { return _present_queue; }
    VmaAllocator const& vma_allocator() const { return _vma_allocator; }

#ifdef NDEBUG
    static const bool enable_validation_layers = false;
#else
    static const bool enable_validation_layers = true;
#endif

  private:
    VkPhysicalDeviceFeatures2 device_features = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    VkPhysicalDeviceVulkan11Features vk11_features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
    VkPhysicalDeviceVulkan12Features vk12_features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    VkPhysicalDeviceVulkan13Features vk13_features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    VkPhysicalDeviceDescriptorBufferFeaturesEXT descriptor_buffer_features{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT};

    static constexpr std::array<const char*, 1> validation_layers = {"VK_LAYER_KHRONOS_validation"};
    static constexpr VkValidationFeatureEnableEXT enabled_validation_features[] = {
        VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
    };

    bool supports_validation_layers();

    std::vector<const char*> get_required_extensions();
    VkDebugUtilsMessengerEXT debug_messenger;
    PFN_vkSetDebugUtilsObjectNameEXT set_debug_utils_object_name;
    void setup_debug_messenger(VkDebugUtilsMessengerCreateInfoEXT debug_create_info);
    VkInstance _instance;
    void create_instance();
    VkSurfaceKHR _surface;
    QueueFamilyIndices find_queue_families(VkPhysicalDevice device);
    std::vector<const char*> device_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    bool use_descriptor_buffers = true;
    bool check_device_extension_support(VkPhysicalDevice device);
    SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device);
    bool check_features(VkPhysicalDevice device);
    bool is_device_suitable(VkPhysicalDevice device);
    VkPhysicalDevice _physical_device = VK_NULL_HANDLE;
    uint32_t _max_subgroup_size;
    uint32_t _max_compute_work_group_invocations;
    VkPhysicalDeviceDescriptorBufferPropertiesEXT _descriptor_buffer_properties{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT};
    float _timestamp_period = 0;
    VkSampleCountFlagBits _msaa_samples = VK_SAMPLE_COUNT_1_BIT;
    static const VkBool32 _msaa_enable = VK_FALSE;
    void pick_physical_device();
    VkDevice _device;
    VkQueue _graphics_queue;
    VkQueue _present_queue;
    void create_logical_device();
    VkCommandPool _command_pool;
    VkCommandPool create_command_pool(VkCommandPoolCreateFlags flags);
    std::vector<VkFence> fence_pool;
    VkFence get_fence();
    void release_fence(VkFence fence);
    VmaAllocator _vma_allocator;

    class CommandPoolAllocator {
      public:
        CommandPoolAllocator();
        ~CommandPoolAllocator();
        VkCommandPool get_pool();
        VkCommandBuffer get_buffer(VkCommandPool pool, VkCommandBufferLevel level);
        VkCommandBuffer get_primary(VkCommandPool pool);
        VkCommandBuffer get_secondary(VkCommandPool pool);
        void release_pool(VkCommandPool pool);
        void release_buffer(VkCommandPool pool, VkCommandBuffer buffer);

      private:
        std::unordered_map<VkCommandPool, std::pair<bool, std::unordered_map<VkCommandBuffer, bool>>> pools;
        static std::mutex get_pool_mutex;
        static std::mutex get_buffer_mutex;
    };
    CommandPoolAllocator* _command_pool_allocator;

  public:
    CommandPoolAllocator* command_pools() { return _command_pool_allocator; };
    void submit_queue(VkQueueFlags queue, std::vector<VkSubmitInfo> submit_infos);
};

#endif // DEVICE_H_
