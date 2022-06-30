#ifndef OPEN4X_H_
#define OPEN4X_H_

#include "vulkan_device.hpp"
#include "vulkan_renderer.hpp"
#include "vulkan_window.hpp"

class Open4X {
    public:
        Open4X();
        ~Open4X();
        void run();
    private:

        VulkanWindow *vulkanWindow;
        VulkanDevice *vulkanDevice;
        VulkanRenderer *vulkanRenderer;

};

#endif // OPEN4X_H_
