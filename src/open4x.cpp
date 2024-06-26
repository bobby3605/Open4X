#include "Vulkan/vulkan_descriptors.hpp"
#include "Vulkan/vulkan_rendergraph.hpp"
#include <chrono>
#include <cstdint>
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtx/dual_quaternion.hpp>
#include <glm/trigonometric.hpp>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
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
#include "rapidjson/istreamwrapper.h"
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

    vulkanDevice = std::make_shared<VulkanDevice>(vulkanWindow);
}

Open4X::~Open4X() {

    //    delete vulkanRenderer;
    //    delete vulkanDevice;
    delete vulkanWindow;
}

void fillComputePushConstants(ComputePushConstants& computePushConstants, float vFov, float aspectRatio, float nearClip, float farClip) {
    computePushConstants.nearD = nearClip;
    computePushConstants.farD = farClip;
    computePushConstants.ratio = aspectRatio;

    vFov *= glm::pi<double>() / 360.0;
    computePushConstants.tang = glm::tan(vFov);
    computePushConstants.sphereFactorY = 1.0 / glm::cos(vFov);

    float angleX = glm::atan(computePushConstants.tang * aspectRatio);
    computePushConstants.sphereFactorX = 1.0 / glm::cos(angleX);
}

void setComputePushConstantsCamera(ComputePushConstants& computePushConstants, VulkanObject* camera) {
    computePushConstants.camPos = camera->position();
    computePushConstants.Z = glm::normalize(camera->rotation() * VulkanObject::forwardVector);
    computePushConstants.X = glm::normalize(camera->rotation() * VulkanObject::rightVector);
    computePushConstants.Y = glm::normalize(camera->rotation() * VulkanObject::upVector);
}

void Open4X::loadSettings() {
    settings = std::make_shared<Settings>();
    std::ifstream file("assets/settings.json");
    if (file.is_open()) {
        IStreamWrapper fileStream(file);
        Document d;
        d.ParseStream(fileStream);
        file.close();

        Value& objectsJSON = d["objects"];
        assert(objectsJSON.IsObject());

        settings->extraObjectCount = objectsJSON["extraObjectCount"].GetInt();
        settings->randLimit = objectsJSON["randLimit"].GetInt();

        Value& miscJSON = d["misc"];
        assert(miscJSON.IsObject());
        settings->showFPS = miscJSON["showFPS"].GetBool();
        settings->pauseOnMinimization = miscJSON["pauseOnMinimization"].GetBool();

    } else {
        std::cout << "Failed to open settings file, using defaults" << std::endl;
    }
}

void Open4X::run() {

    loadSettings();

    VulkanRenderGraph renderGraph(vulkanDevice, vulkanWindow, settings);

    VulkanObjects objects(vulkanDevice, &renderGraph, settings);

    camera = new VulkanObject();
    //    camera->children.push_back(objects.getObjectByName("assets/glTF/uss_enterprise_d_star_trek_tng.glb"));

    UniformBufferObject ubo{};

    float vFov = 45.0f;
    float nearClip = 0.0001f;
    float farClip = 1000.0f;
    float aspectRatio = renderGraph.getSwapChainExtent().width / (float)renderGraph.getSwapChainExtent().height;

    glm::mat4 proj = perspectiveProjection(vFov, aspectRatio, nearClip, farClip);

    objects.computePushConstants.totalInstanceCount = objects.totalInstanceCount();
    fillComputePushConstants(objects.computePushConstants, vFov, aspectRatio, nearClip, farClip);

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

        glm::mat4 cameraModel =
            glm::translate(glm::mat4(1.0f), camera->position()) * glm::toMat4(camera->rotation()) * glm::scale(camera->scale());

        ubo.projView = proj * glm::inverse(cameraModel);
        ubo.camPos = camera->position();

        renderGraph.bufferWrite("Globals", &ubo);

        setComputePushConstantsCamera(objects.computePushConstants, camera);

        objects.updateModels();

        if (renderGraph.render()) {
            aspectRatio = renderGraph.getSwapChainExtent().width / (float)renderGraph.getSwapChainExtent().height;
            proj = perspectiveProjection(vFov, aspectRatio, nearClip, farClip);
            fillComputePushConstants(objects.computePushConstants, vFov, aspectRatio, nearClip, farClip);
        }

        //        std::this_thread::sleep_for(std::chrono::seconds(1));

        if (settings->showFPS) {
            const uint32_t queryCount = 4;
            std::vector<uint64_t> queryResults(queryCount);
            const bool wait = true;
            // FIXME:
            // workaround for disabling waiting on some gpus
            // TODO:
            // One query pool per frame
            vkGetQueryPoolResults(vulkanDevice->device(), objects.queryPool, 0, queryCount, queryResults.size() * sizeof(queryResults[0]),
                                  queryResults.data(), sizeof(queryResults[0]),
                                  VK_QUERY_RESULT_64_BIT | (wait ? VK_QUERY_RESULT_WAIT_BIT : VK_QUERY_RESULT_64_BIT));

            float cullTime = (queryResults[1] - queryResults[0]) * vulkanDevice->timestampPeriod() * 1e-6;
            float drawTime = (queryResults[3] - queryResults[2]) * vulkanDevice->timestampPeriod() * 1e-6;

            std::stringstream title;
            title << "Frametime: " << std::fixed << std::setprecision(2) << (titleFrametime * 1000) << "ms"
                  << " "
                  << "Framerate: " << 1.0 / titleFrametime << " "
                  << "Drawtime: " << drawTime << "ms"
                  << " "
                  << "Culltime: " << cullTime << "ms";

            glfwSetWindowTitle(vulkanWindow->getGLFWwindow(), title.str().c_str());
        }
    }
    vkDeviceWaitIdle(vulkanDevice->device());
    delete camera;
}
