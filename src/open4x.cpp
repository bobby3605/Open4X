#include "Vulkan/vulkan_descriptors.hpp"
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "Vulkan/common.hpp"
#include "Vulkan/vulkan_buffer.hpp"
#include "Vulkan/vulkan_object.hpp"
#include "Vulkan/vulkan_objects.hpp"
#include "Vulkan/vulkan_swapchain.hpp"
#include "open4x.hpp"
#include <GLFW/glfw3.h>
#include <cstring>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/transform.hpp>
#include <iostream>

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, 1);
    }
}

glm::mat4 perspectiveProjection(float vertical_fov, float aspect_ratio, float near, float far) {
    assert(glm::abs(aspect_ratio - std::numeric_limits<float>::epsilon()) > 0.0f);
    const float tanHalfFovy = tan(glm::radians(vertical_fov) / 2.f);
    glm::mat4 projectionMatrix{0.0f};
    projectionMatrix[0][0] = 1.f / (aspect_ratio * tanHalfFovy);
    projectionMatrix[1][1] = 1.f / (tanHalfFovy);
    projectionMatrix[2][2] = far / (far - near);
    projectionMatrix[2][3] = 1.f;
    projectionMatrix[3][2] = -(far * near) / (far - near);
    return projectionMatrix;
}

Open4X::Open4X() {
    creationTime = std::chrono::high_resolution_clock::now();
    vulkanWindow = new VulkanWindow(640, 480, "Open 4X");
    glfwSetKeyCallback(vulkanWindow->getGLFWwindow(), key_callback);

    vulkanDevice = new VulkanDevice(vulkanWindow);
}

Open4X::~Open4X() {

    delete vulkanRenderer;
    delete vulkanDevice;
    delete vulkanWindow;
}

void Open4X::run() {

    VulkanDescriptors descriptorManager(vulkanDevice);

    VulkanObjects objects(vulkanDevice, &descriptorManager);
    vulkanRenderer = new VulkanRenderer(vulkanWindow, vulkanDevice, &descriptorManager);

    std::vector<VkDescriptorSet> globalSets;

    descriptorManager.createSets(descriptorManager.getGlobal(), globalSets);

    std::vector<UniformBuffer*> uniformBuffers(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);
    std::vector<VkDescriptorBufferInfo> bufferInfos(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);

    for (int i = 0; i < VulkanSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        uniformBuffers[i] = new UniformBuffer(vulkanDevice, sizeof(UniformBufferObject));
        bufferInfos[i] = uniformBuffers[i]->getBufferInfo();

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = globalSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfos[i];

        vkUpdateDescriptorSets(vulkanDevice->device(), 1, &descriptorWrite, 0, nullptr);
    }

    camera = new VulkanObject();
    //    camera->children.push_back(objects.getObjectByName("assets/glTF/uss_enterprise_d_star_trek_tng.glb"));

    UniformBufferObject ubo{};

    ubo.proj = perspectiveProjection(45.0f, vulkanRenderer->getSwapChainExtent().width / (float)vulkanRenderer->getSwapChainExtent().height,
                                     0.001f, 1000.0f);

    auto startTime = std::chrono::high_resolution_clock::now();
    std::cout << "Total load time: " << std::chrono::duration<float, std::chrono::milliseconds::period>(startTime - creationTime).count()
              << "ms" << std::endl;
    float titleFrametime = 0.0f;
    std::string title;
    while (!glfwWindowShouldClose(vulkanWindow->getGLFWwindow())) {
        glfwPollEvents();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        startTime = currentTime;
        camera->keyboardUpdate(vulkanWindow->getGLFWwindow(), frameTime);
        if (titleFrametime == 0.0f) {
            titleFrametime = frameTime;
        } else {
            titleFrametime = titleFrametime * 0.95 + frameTime * 0.05;
        }
        title = "Frametime: " + std::to_string(titleFrametime * 1000) + " ms" + " Framerate: " + std::to_string(1.0 / titleFrametime);
        glfwSetWindowTitle(vulkanWindow->getGLFWwindow(), title.c_str());

        glm::mat4 cameraModel =
            glm::translate(glm::mat4(1.0f), camera->position()) * glm::toMat4(camera->rotation()) * glm::scale(camera->scale());

        ubo.view = glm::inverse(cameraModel);

        vulkanRenderer->startFrame();

        uniformBuffers[vulkanRenderer->getCurrentFrame()]->write(&ubo);

        vulkanRenderer->bindComputePipeline();
        vulkanRenderer->runComputePipeline(objects.computeSet, objects.indirectDrawCount(), objects.drawIndirectCountBuffer());

        vulkanRenderer->beginRendering();

        vulkanRenderer->bindPipeline();

        vulkanRenderer->bindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanRenderer->graphicsPipelineLayout(), 0,
                                          globalSets[vulkanRenderer->getCurrentFrame()]);

        // TODO
        // add support for switching to direct drawing
        objects.bind(vulkanRenderer);

        objects.drawIndirect(vulkanRenderer);

        vulkanRenderer->endRendering();

        if (vulkanRenderer->endFrame()) {
            ubo.proj = perspectiveProjection(
                45.0f, vulkanRenderer->getSwapChainExtent().width / (float)vulkanRenderer->getSwapChainExtent().height, 0.001f, 1000.0f);
        }
    }
    vkDeviceWaitIdle(vulkanDevice->device());
    delete camera;
    for (size_t i = 0; i < VulkanSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        delete uniformBuffers[i];
    }
}
