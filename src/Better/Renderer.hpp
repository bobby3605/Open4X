#ifndef RENDERER_H_
#define RENDERER_H_
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Renderer {
    public:
        virtual ~Renderer() = default;
        virtual void drawFrame() = 0;
        GLFWwindow *window;
};

#endif // RENDERER_H_
