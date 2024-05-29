#include "device.hpp"
#include "common.hpp"
#include "window.hpp"
#include <iostream>
#include <set>
#include <vulkan/vulkan_core.h>

Device* Device::device;
Device::Device() {
    set_required_features();
    create_instance();
    check_result(glfwCreateWindowSurface(_instance, Window::window->glfw_window(), nullptr, &_surface), "failed to create window surface");
    pick_physical_device();
    create_logical_device();
    _command_pool = create_command_pool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    _command_pool_allocator = new CommandPoolAllocator();
    device = this;
    // FIXME:
    // Replace with volk
    load_device_addr(vkGetDescriptorSetLayoutSizeEXT, _device);
    load_device_addr(vkGetDescriptorSetLayoutBindingOffsetEXT, _device);
    load_device_addr(vkGetDescriptorEXT, _device);
    load_device_addr(vkCmdBindDescriptorBuffersEXT, _device);
    load_device_addr(vkCmdSetDescriptorBufferOffsets2EXT, _device);

    VmaVulkanFunctions vulkan_functions = {};
    vulkan_functions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
    vulkan_functions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocatorCreateInfo = {};
    allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT | VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    allocatorCreateInfo.vulkanApiVersion = Device::API_VERSION;
    allocatorCreateInfo.physicalDevice = Device::device->physical_device();
    allocatorCreateInfo.device = Device::device->vk_device();
    allocatorCreateInfo.instance = Device::device->instance();
    allocatorCreateInfo.pVulkanFunctions = &vulkan_functions;

    vmaCreateAllocator(&allocatorCreateInfo, &_vma_allocator);
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

Device::~Device() {
    for (VkFence& fence : fence_pool) {
        vkDestroyFence(_device, fence, nullptr);
    }
    vkDestroyCommandPool(_device, _command_pool, nullptr);
    delete _command_pool_allocator;
    vmaDestroyAllocator(_vma_allocator);
    vkDestroyDevice(_device, nullptr);

    if (enable_validation_layers) {
        DestroyDebugUtilsMessengerEXT(_instance, debug_messenger, nullptr);
    }

    vkDestroySurfaceKHR(_instance, _surface, nullptr);
    vkDestroyInstance(_instance, nullptr);
}

bool Device::supports_validation_layers() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validation_layers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }
        if (!layerFound) {
            return false;
        }
    }

    return true;
}

std::vector<const char*> Device::get_required_extensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enable_validation_layers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                              VkDebugUtilsMessageTypeFlagsEXT messageType,
                                              const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

void Device::create_instance() {

    if (enable_validation_layers && !supports_validation_layers()) {
        throw std::runtime_error("validation layers requested, but not available");
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = Window::window->name().c_str();
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = (Window::window->name() + " Engine").c_str();
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = API_VERSION;

    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &appInfo;

    auto extensions = get_required_extensions();
    create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debug_create_info{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    VkValidationFeaturesEXT validation_features = {VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT};
    validation_features.enabledValidationFeatureCount = sizeof(enabled_validation_features) / sizeof(enabled_validation_features[0]);
    validation_features.pEnabledValidationFeatures = enabled_validation_features;

    if (enable_validation_layers) {
        create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
        create_info.ppEnabledLayerNames = validation_layers.data();

        debug_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug_create_info.pfnUserCallback = debug_callback;
        debug_create_info.pUserData = VK_NULL_HANDLE;

        create_info.pNext = &debug_create_info;
    } else {
        create_info.enabledLayerCount = 0;

        create_info.pNext = nullptr;
    }
    check_result(vkCreateInstance(&create_info, nullptr, &_instance), "failed to create instance");
    if (enable_validation_layers) {
        setup_debug_messenger(debug_create_info);
        debug_create_info.pNext = &validation_features;
    }
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                      const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void Device::setup_debug_messenger(VkDebugUtilsMessengerCreateInfoEXT debug_create_info) {

    check_result(CreateDebugUtilsMessengerEXT(_instance, &debug_create_info, nullptr, &debug_messenger),
                 "failed to set up debug messenger");

    set_debug_utils_object_name =
        reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetInstanceProcAddr(_instance, "vkSetDebugUtilsObjectNameEXT"));
}

Device::QueueFamilyIndices Device::find_queue_families(VkPhysicalDevice device) {
    QueueFamilyIndices indices;

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

    int i = 0;
    for (const auto& queue_family : queue_families) {
        // FIXME:
        // Update rendergraph to allow for different queues
        // Need one command buffer per queue
        if (queue_family.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) {
            indices.graphicsFamily = i;
        }

        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, _surface, &present_support);

        if (present_support) {
            indices.presentFamily = i;
        }

        if (indices.is_complete()) {
            break;
        }

        i++;
    }

    return indices;
}

bool Device::check_device_extension_support(VkPhysicalDevice device) {
    uint32_t extension_count;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

    std::vector<VkExtensionProperties> available_extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

    std::set<std::string> required_extensions(device_extensions.begin(), device_extensions.end());

    for (const auto& extension : available_extensions) {
        required_extensions.erase(extension.extensionName);
    }

    return required_extensions.empty();
}

Device::SwapChainSupportDetails Device::query_swap_chain_support(VkPhysicalDevice device) {
    SwapChainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, _surface, &details.capabilities);

    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &format_count, nullptr);
    if (format_count != 0) {
        details.formats.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &format_count, details.formats.data());
    }

    uint32_t present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &present_mode_count, nullptr);

    if (present_mode_count != 0) {
        details.presentModes.resize(present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &present_mode_count, details.presentModes.data());
    }

    return details;
}

void Device::set_required_features() {
    device_features.features.samplerAnisotropy = VK_TRUE;
    device_features.features.sampleRateShading = sample_shading;
    device_features.features.multiDrawIndirect = VK_TRUE;
    device_features.features.shaderFloat64 = VK_TRUE;

    vk11_features.shaderDrawParameters = VK_TRUE;

    vk12_features.runtimeDescriptorArray = VK_TRUE;
    vk12_features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    vk12_features.drawIndirectCount = VK_TRUE;
    vk12_features.hostQueryReset = VK_TRUE;
    vk12_features.scalarBlockLayout = VK_TRUE;
    vk12_features.bufferDeviceAddress = VK_TRUE;
    // Only enable when in debug for performance reasons
    vk12_features.bufferDeviceAddressCaptureReplay = enable_validation_layers ? VK_TRUE : VK_FALSE;

    vk13_features.dynamicRendering = VK_TRUE;
    vk13_features.synchronization2 = VK_TRUE;
    vk13_features.subgroupSizeControl = VK_TRUE;
    vk13_features.maintenance4 = VK_TRUE;

    descriptor_buffer_features.descriptorBuffer = VK_TRUE;
    // NOTE:
    // Neither mesa nor renderdoc support this
    // amdgpu does, but without renderdoc support it's useless
    // Until capture replay works in mesa and renderdoc,
    // descriptor buffers are dead in the water for development
    //    descriptor_buffer_features.descriptorBufferCaptureReplay = enable_validation_layers ? VK_TRUE : VK_FALSE;

    device_features.pNext = &vk11_features;
    vk11_features.pNext = &vk12_features;
    vk12_features.pNext = &vk13_features;
    vk13_features.pNext = &descriptor_buffer_features;
}

// Macro abuse to try to make comparing device has and must have features a little easier
// TODO
// Better solution to this

// min_have and device_has are the same, or min have is false
#define comp(min_have_name, device_has_name, feature_name)                                                                                 \
    ((min_have_name.feature_name == device_has_name.feature_name) || !min_have_name.feature_name)

#define comp_check(var_name, feature_name) comp(var_name, var_name##_check, feature_name)

#define compdf(feature_name) comp_check(device_features, feature_name)
#define comp11(feature_name) comp_check(vk11_features, feature_name)
#define comp12(feature_name) comp_check(vk12_features, feature_name)
#define comp13(feature_name) comp_check(vk13_features, feature_name)
#define compdb(feature_name) comp_check(descriptor_buffer_features, feature_name)

bool Device::check_features(VkPhysicalDevice device) {
    VkPhysicalDeviceDescriptorBufferFeaturesEXT descriptor_buffer_features_check{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT};

    VkPhysicalDeviceVulkan13Features vk13_features_check{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};

    VkPhysicalDeviceVulkan12Features vk12_features_check{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    vk12_features_check.pNext = &vk13_features_check;

    VkPhysicalDeviceVulkan11Features vk11_features_check{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
    vk11_features_check.pNext = &vk12_features_check;

    VkPhysicalDeviceFeatures2 device_features_check{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    device_features_check.pNext = &vk11_features_check;

    vkGetPhysicalDeviceFeatures2(device, &device_features_check);

    bool device_features_available = compdf(features.samplerAnisotropy) && compdf(features.sampleRateShading) &&
                                     compdf(features.multiDrawIndirect) && compdf(features.shaderFloat64);

    bool vk11_features_available = comp11(shaderDrawParameters);

    bool vk12_features_available = comp12(runtimeDescriptorArray) && comp12(shaderSampledImageArrayNonUniformIndexing) &&
                                   comp12(drawIndirectCount) && comp12(hostQueryReset) && comp12(scalarBlockLayout) &&
                                   comp12(bufferDeviceAddress) && comp12(bufferDeviceAddressCaptureReplay);

    bool vk13_features_available =
        comp13(dynamicRendering) && comp13(synchronization2) && comp13(subgroupSizeControl) && comp13(maintenance4);

    bool descriptor_buffer_features_available = compdb(descriptorBuffer); //&& compdb(descriptorBufferCaptureReplay);

    return device_features_available && vk11_features_available && vk12_features_available && vk13_features_available;
}

bool Device::is_device_suitable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = find_queue_families(device);

    bool extensions_supported = check_device_extension_support(device);

    bool swap_chain_adequate = false;
    if (extensions_supported) {
        SwapChainSupportDetails swap_chain_support = query_swap_chain_support(device);
        swap_chain_adequate = !swap_chain_support.formats.empty() && !swap_chain_support.presentModes.empty();
    }

    return indices.is_complete() && extensions_supported && swap_chain_adequate && check_features(device);
}

VkSampleCountFlagBits get_max_usable_sample_count(VkPhysicalDeviceProperties props) {
    VkSampleCountFlags counts = props.limits.framebufferColorSampleCounts & props.limits.framebufferDepthSampleCounts;

    if (counts & VK_SAMPLE_COUNT_64_BIT) {
        return VK_SAMPLE_COUNT_64_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_32_BIT) {
        return VK_SAMPLE_COUNT_32_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_16_BIT) {
        return VK_SAMPLE_COUNT_16_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_8_BIT) {
        return VK_SAMPLE_COUNT_8_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_4_BIT) {
        return VK_SAMPLE_COUNT_4_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_2_BIT) {
        return VK_SAMPLE_COUNT_2_BIT;
    }

    return VK_SAMPLE_COUNT_1_BIT;
}

void Device::pick_physical_device() {

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(_instance, &deviceCount, VK_NULL_HANDLE);

    if (deviceCount == 0)
        throw std::runtime_error("failed to find GPUs with Vulkan support");
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());

    for (const auto& device : devices) {
        if (is_device_suitable(device)) {
            _physical_device = device;
            break;
        }
    }

    if (_physical_device == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU");
    }

    VkPhysicalDeviceVulkan13Properties vk13_properties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES};
    VkPhysicalDeviceProperties2 properties2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    properties2.pNext = &vk13_properties;
    vk13_properties.pNext = &_descriptor_buffer_properties;
    vkGetPhysicalDeviceProperties2(_physical_device, &properties2);
    std::cout << "Using device: " << properties2.properties.deviceID << " " << properties2.properties.deviceName << std::endl;

    _max_subgroup_size = vk13_properties.maxSubgroupSize;
    _timestamp_period = properties2.properties.limits.timestampPeriod;
    _max_compute_work_group_invocations = properties2.properties.limits.maxComputeWorkGroupInvocations;

    if (_msaa_enable == VK_TRUE)
        _msaa_samples = get_max_usable_sample_count(properties2.properties);
    std::cout << "MSAA Samples: " << _msaa_samples << std::endl;
}

void Device::create_logical_device() {
    QueueFamilyIndices indices = find_queue_families(_physical_device);

    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    std::set<uint32_t> unique_queue_families = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    float queue_priority = 1.0f;
    for (uint32_t queue_family : unique_queue_families) {
        VkDeviceQueueCreateInfo queue_create_info{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        queue_create_info.queueFamilyIndex = queue_family;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &queue_priority;
        queue_create_infos.push_back(queue_create_info);
    }

    VkDeviceCreateInfo create_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
    create_info.pQueueCreateInfos = queue_create_infos.data();
    create_info.pNext = &device_features;

    create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
    create_info.ppEnabledExtensionNames = device_extensions.data();

    if (enable_validation_layers) {
        create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
        create_info.ppEnabledLayerNames = validation_layers.data();
    } else {
        create_info.enabledLayerCount = 0;
    }

    check_result(vkCreateDevice(_physical_device, &create_info, nullptr, &_device), "failed to create a logical device");

    vkGetDeviceQueue(_device, indices.graphicsFamily.value(), 0, &_graphics_queue);
    vkGetDeviceQueue(_device, indices.presentFamily.value(), 0, &_present_queue);
}

VkCommandPool Device::create_command_pool(VkCommandPoolCreateFlags flags) {
    QueueFamilyIndices queue_family_indices = find_queue_families(_physical_device);

    VkCommandPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pool_info.flags = flags;
    pool_info.queueFamilyIndex = queue_family_indices.graphicsFamily.value();

    VkCommandPool pool;
    check_result(vkCreateCommandPool(_device, &pool_info, nullptr, &pool), "failed to create command pool");
    return pool;
}

void Device::set_debug_name(VkObjectType type, uint64_t handle, std::string name) {
    if (enable_validation_layers) {
        VkDebugUtilsObjectNameInfoEXT name_info{VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
        name_info.pNext = VK_NULL_HANDLE;
        name_info.objectType = type;
        name_info.objectHandle = handle;
        name_info.pObjectName = name.c_str();

        set_debug_utils_object_name(_device, &name_info);
    }
}

VkImageView Device::create_image_view(std::string name, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags,
                                      uint32_t mipLevels) {
    VkImageViewCreateInfo view_info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    view_info.image = image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = format;
    view_info.subresourceRange.aspectMask = aspectFlags;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = mipLevels;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;

    VkImageView image_view;
    check_result(vkCreateImageView(_device, &view_info, nullptr, &image_view), "failed to create image view!");
    set_debug_name(VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)image_view, name);

    return image_view;
}

VkFence Device::get_fence() {
    static const uint32_t createCount = 10;
    VkFence fence;
    if (fence_pool.size() == 0) {
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (int i = 0; i < createCount; i++) {
            check_result(vkCreateFence(_device, &fenceInfo, nullptr, &fence), "failed to create single time fence");
            fence_pool.push_back(fence);
        }
        vkResetFences(_device, createCount, fence_pool.data());
    }
    fence = fence_pool.back();
    fence_pool.pop_back();
    return fence;
}

void Device::release_fence(VkFence fence) {
    check_result(vkResetFences(_device, 1, &fence), "failed to release fence");
    fence_pool.push_back(fence);
}

void Device::submit_queue(VkQueueFlags queue, std::vector<VkSubmitInfo> submit_infos) {

    VkFence fence = get_fence();
    vkQueueSubmit(_graphics_queue, submit_infos.size(), submit_infos.data(), fence);
    vkWaitForFences(_device, 1, &fence, VK_TRUE, UINT64_MAX);
    release_fence(fence);
}
