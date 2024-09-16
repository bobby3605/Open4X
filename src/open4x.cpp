#include "open4x.hpp"
#include "Renderer/Allocator/base_allocator.hpp"
#include "Renderer/Allocator/sub_allocator.hpp"
#include "Renderer/Vulkan/camera.hpp"
#include "Renderer/Vulkan/common.hpp"
#include "Renderer/Vulkan/memory_manager.hpp"
#include "Renderer/Vulkan/object_manager.hpp"
#include "Renderer/Vulkan/rendergraph.hpp"
#include "Vulkan/common.hpp"
#include "Vulkan/vulkan_buffer.hpp"
#include "Vulkan/vulkan_descriptors.hpp"
#include "Vulkan/vulkan_object.hpp"
#include "Vulkan/vulkan_objects.hpp"
#include "Vulkan/vulkan_rendergraph.hpp"
#include "rapidjson/istreamwrapper.h"
#include "utils/utils.hpp"
#include <GLFW/glfw3.h>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <future>
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/dual_quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/trigonometric.hpp>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#define NEW_RENDERER 1

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
    srand(time(NULL));
    creationTime = std::chrono::high_resolution_clock::now();
    // ensure cache directories exist
    std::string directory = "assets/cache/shaders/";
    std::filesystem::create_directories(directory);
    directory = "assets/cache/images/";
    std::filesystem::create_directories(directory);
    loadSettings();
    if (NEW_RENDERER) {
        new Window(640, 480, "Open 4X");
        glfwSetKeyCallback(Window::window->glfw_window(), key_callback);
        renderer = new Renderer((NewSettings*)settings.get());
        _model_manager = new ModelManager(renderer->draw_allocators);
        _object_manager = new ObjectManager();
    } else {
        vulkanWindow = new VulkanWindow(640, 480, "Open 4X");
        glfwSetKeyCallback(vulkanWindow->getGLFWwindow(), key_callback);

        vulkanDevice = std::make_shared<VulkanDevice>(vulkanWindow);
    }
}

Open4X::~Open4X() {
    if (NEW_RENDERER) {
        delete _object_manager;
        delete _model_manager;
        delete renderer;
        delete Window::window;
    } else {
        //    delete vulkanRenderer;
        //    delete vulkanDevice;
        delete vulkanWindow;
    }
}

/*
    uint totalInstanceCount;
    float nearD;
    float farD;
    float ratio;
    float sphereFactorX;
    float sphereFactorY;
    float tang;
    vec3 X;
    vec3 Y;
    vec3 Z;
    vec3 camPos;
  */

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

void setFrustumConsts(float const& vertical_fov, float const& aspect_ratio, float const& near_clip, float const& far_clip,
                      RenderGraph* rg) {
    PtrWriter writer(rg->get_push_constant("frustum_consts") + sizeof(uint));
    writer.write(near_clip);
    writer.write(far_clip);
    writer.write(aspect_ratio);

    float vfov_rad = vertical_fov * (glm::pi<double>() / 360.0);
    float tang = glm::tan(vfov_rad);

    float angle_X = glm::atan(tang * aspect_ratio);
    writer.write(1.0f / glm::cos(angle_X));
    writer.write(1.0f / glm::cos(vfov_rad));
    writer.write(tang);
}

void setFrustumCamConsts(Camera* cam, RenderGraph* rg) {
    PtrWriter writer(rg->get_push_constant("frustum_consts") + sizeof(uint) + 6 * sizeof(float));
    writer.write(glm::normalize(cam->rotation() * VulkanObject::rightVector));
    writer.write(glm::normalize(cam->rotation() * VulkanObject::upVector));
    writer.write(glm::normalize(cam->rotation() * VulkanObject::forwardVector));
    writer.write(cam->position());
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

        new_settings = new NewSettings;
        new_settings->extra_object_count = settings->extraObjectCount;
        new_settings->rand_limit = settings->randLimit;
        new_settings->show_fps = settings->showFPS;
        new_settings->pause_on_minimization = settings->pauseOnMinimization;
        new_settings->object_refresh_threads = miscJSON["object_refresh_threads"].GetInt();
        new_settings->object_bulk_create_threads = miscJSON["object_bulk_create_threads"].GetInt();
        new_settings->invalid_draws_refresh_threads = miscJSON["invalid_draws_refresh_threads"].GetInt();

    } else {
        std::cout << "Failed to open settings file, using defaults" << std::endl;
    }
}

void Open4X::run() {
    if (NEW_RENDERER) {
        std::string assets_base_path = std::filesystem::current_path().string() + "/assets/glTF/";

        // NOTE:
        // GPU buffers don't exist until you allocate from them,
        // so the engine runs without any models/objects allocated,
        // then there are validation errors for missing buffers

        Model* box_model = _model_manager->get_model(assets_base_path + "Box.gltf");
        _object_manager->create_n_objects(box_model, new_settings->extra_object_count);

        Model* engine_model = _model_manager->get_model(assets_base_path + "2CylinderEngine.glb");
        size_t engine_id = _object_manager->add_object(engine_model);
        Object* engine_obj = _object_manager->get_object(engine_id);

        engine_obj->scale({0.01, 0.01, 0.01});
        engine_obj->position({5, 0, 0});

        Model* simple_texture_model = _model_manager->get_model(assets_base_path + "simple_texture.gltf");
        size_t simple_texture_id = _object_manager->add_object(simple_texture_model);
        Object* simple_texture_obj = _object_manager->get_object(simple_texture_id);
        simple_texture_obj->rotation_euler(0, 180, 180);

        Model* water_bottle_model = _model_manager->get_model(assets_base_path + "WaterBottle.glb");
        size_t water_bottle_id = _object_manager->add_object(water_bottle_model);
        Object* water_bottle_obj = _object_manager->get_object(water_bottle_id);

        water_bottle_obj->position({0, -2, 0});
        water_bottle_obj->scale({5, 5, 5});
        water_bottle_obj->rotation_euler(0, 0, 180);

        size_t water_bottle_light_id = _object_manager->add_object(water_bottle_model);
        Object* water_bottle_light_obj = _object_manager->get_object(water_bottle_light_id);

        water_bottle_light_obj->position({0, 2, 1});

        Model* a_beautiful_game_model = _model_manager->get_model(assets_base_path + "ABeautifulGame/ABeautifulGame.gltf");
        size_t a_beautiful_game_id = _object_manager->add_object(a_beautiful_game_model);
        Object* a_beautiful_game_object = _object_manager->get_object(a_beautiful_game_id);

        a_beautiful_game_object->position({-2, -2, -2});
        a_beautiful_game_object->rotation_euler(180, 0, 0);
        a_beautiful_game_object->scale({2, 2, 2});

        Model* simple_animation = _model_manager->get_model(assets_base_path + "simple_animation.gltf");
        size_t simple_animation_id = _object_manager->add_object(simple_animation);
        Object* simple_animation_object = _object_manager->get_object(simple_animation_id);
        simple_animation_object->position({2, 5, -4});
        simple_animation_object->rotation_euler(0, 180, 0);

        Model* box_animated = _model_manager->get_model(assets_base_path + "BoxAnimated.glb");
        size_t box_animated_id = _object_manager->add_object(box_animated);
        Object* box_animated_object = _object_manager->get_object(box_animated_id);
        box_animated_object->position({2, 5, 2});
        box_animated_object->rotation_euler(180, 0, 0);

        std::vector<Light*> lights;
        lights.resize(3);
        Model* duck_model = _model_manager->get_model(assets_base_path + "Duck.glb");
        for (size_t i = 0; i < lights.size(); ++i) {
            lights[i] = _object_manager->get_light(_object_manager->add_light(duck_model));
            lights[i]->rotation_euler(180, 0, 0);
        }
        lights[0]->position({0.0, 1.0, -2.0});
        lights[1]->position({0.0, 1.0, 5.0});
        lights[2]->position({-2.0, -3.0, -2.0});
        lights[0]->color(10.0f * lights[0]->color());
        lights[1]->color(10.0f * lights[1]->color());

        std::optional<size_t> blue_box_id;

        float vertical_fov = 45.0f;
        float near = 0.0001f;
        float far = 1000.0f;
        VkExtent2D extent = Window::window->extent();
        float aspect_ratio = (float)extent.width / (float)extent.height;

        Camera cam(vertical_fov, aspect_ratio, near, far);
        setFrustumConsts(vertical_fov, aspect_ratio, near, far, renderer->rg);

        FixedAllocator<GPUAllocation>* shader_globals_allocator = new FixedAllocator(
            sizeof(ShaderGlobals),
            gpu_allocator->create_buffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, "Globals"));

        SubAllocation<FixedAllocator, GPUAllocation>* globals_alloc = shader_globals_allocator->alloc();
        ShaderGlobals shader_globals;
        auto refresh_start = std::chrono::high_resolution_clock::now();
        _object_manager->refresh_invalid_objects();
        _model_manager->refresh_invalid_draws();
        std::cout << "initial refresh time: "
                  << std::chrono::duration<float, std::chrono::milliseconds::period>(std::chrono::high_resolution_clock::now() -
                                                                                     refresh_start)
                         .count()
                  << "ms" << std::endl;

        auto start_time = std::chrono::high_resolution_clock::now();
        std::cout << "Total load time: "
                  << std::chrono::duration<float, std::chrono::milliseconds::period>(start_time - creationTime).count() << "ms"
                  << std::endl;

        float title_frame_time = 0.0f;
        std::chrono::time_point<std::chrono::system_clock> animation_base_time = std::chrono::high_resolution_clock::now();
        while (!glfwWindowShouldClose(Window::window->glfw_window())) {
            auto current_time = std::chrono::high_resolution_clock::now();
            float frame_time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

            if (title_frame_time == 0.0f) {
                title_frame_time = frame_time;
            } else {
                title_frame_time = title_frame_time * 0.98 + frame_time * 0.02;
            }
            start_time = current_time;
            glfwPollEvents();
            if (glfwGetKey(Window::window->glfw_window(), GLFW_KEY_F2) == GLFW_PRESS) {
                if (!blue_box_id.has_value()) {
                    blue_box_id = _object_manager->add_object(_model_manager->get_model(assets_base_path + "BlueBox.gltf"));
                    _object_manager->get_object(blue_box_id.value())->position({1, 1, 1});
                }
            }
            if (glfwGetKey(Window::window->glfw_window(), GLFW_KEY_F3) == GLFW_PRESS) {
                if (blue_box_id.has_value()) {
                    _object_manager->remove_object(blue_box_id.value());
                    _model_manager->remove_model(assets_base_path + "BlueBox.gltf");
                    blue_box_id.reset();
                }
            }

            uint64_t animation_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - animation_base_time).count();
            _model_manager->refresh_animations(animation_time_ms);
            _object_manager->refresh_invalid_objects();
            _object_manager->refresh_animated_objects();
            _model_manager->refresh_invalid_draws();
            cam.update_transform(frame_time);
            setFrustumCamConsts(&cam, renderer->rg);
            // TODO
            // cache camera matrix
            shader_globals.proj_view = cam.proj_view();
            shader_globals.cam_pos = cam.position();
            globals_alloc->write(&shader_globals);
            renderer->rg->set_push_constant("triangle_frag", _object_manager->light_count());

            // returns true when swapchain was resized
            if (renderer->render()) {
                extent = Window::window->extent();
                aspect_ratio = (float)extent.width / (float)extent.height;
                // adjust perspective projection matrix
                cam.update_projection(vertical_fov, aspect_ratio, near, far);
                setFrustumConsts(vertical_fov, aspect_ratio, near, far, renderer->rg);
            }
            if (settings->showFPS) {
                std::stringstream title;
                title << "Objects: " << _object_manager->object_count() << " "
                      << "Frametime: " << std::fixed << std::setprecision(2) << (title_frame_time * 1000) << "ms"
                      << " "
                      << "Framerate: " << 1.0 / title_frame_time;
                glfwSetWindowTitle(Window::window->glfw_window(), title.str().c_str());
            }
        }
        vkDeviceWaitIdle(Device::device->vk_device());
    } else {
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
        std::cout << "Total load time: "
                  << std::chrono::duration<float, std::chrono::milliseconds::period>(startTime - creationTime).count() << "ms" << std::endl;
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
                vkGetQueryPoolResults(vulkanDevice->device(), objects.queryPool, 0, queryCount,
                                      queryResults.size() * sizeof(queryResults[0]), queryResults.data(), sizeof(queryResults[0]),
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
}
