#include "open4x.hpp"
#include "Renderer/Allocator/allocation.hpp"
#include "Renderer/Allocator/base_allocator.hpp"
#include "Renderer/Allocator/sub_allocator.hpp"
#include "Renderer/Vulkan/camera.hpp"
#include "Renderer/Vulkan/common.hpp"
#include "Renderer/Vulkan/object_manager.hpp"
#include "Renderer/Vulkan/rendergraph.hpp"
#include "Renderer/Vulkan/window.hpp"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stream.h"
#include <GLFW/glfw3.h>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
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
#include <sstream>
#include <string>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

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
    creation_time = std::chrono::high_resolution_clock::now();
    // ensure cache directories exist
    std::string directory = "assets/cache/shaders/";
    std::filesystem::create_directories(directory);
    directory = "assets/cache/images/";
    std::filesystem::create_directories(directory);
    load_settings();
    new Window(640, 480, "Open 4X");
    glfwSetKeyCallback(Window::window->glfw_window(), key_callback);
    renderer = new Renderer(settings);
    _model_manager = new ModelManager(renderer->draw_allocators);
    _object_manager = new ObjectManager();
}

Open4X::~Open4X() {
    delete settings;
    delete _object_manager;
    delete _model_manager;
    delete renderer;
    delete Window::window;
}

void Open4X::load_settings() {
    std::ifstream file("assets/settings.json");
    if (file.is_open()) {
        rapidjson::IStreamWrapper fileStream(file);
        rapidjson::Document d;
        d.ParseStream(fileStream);
        file.close();

        rapidjson::Value& objectsJSON = d["objects"];
        assert(objectsJSON.IsObject());

        rapidjson::Value& miscJSON = d["misc"];
        assert(miscJSON.IsObject());

        settings = new Settings;
        settings->extra_object_count = objectsJSON["extraObjectCount"].GetInt();
        settings->rand_limit = objectsJSON["randLimit"].GetInt();
        settings->show_fps = miscJSON["showFPS"].GetBool();
        settings->pause_on_minimization = miscJSON["pauseOnMinimization"].GetBool();
        settings->object_refresh_threads = miscJSON["object_refresh_threads"].GetInt();
        settings->object_bulk_create_threads = miscJSON["object_bulk_create_threads"].GetInt();
        settings->invalid_draws_refresh_threads = miscJSON["invalid_draws_refresh_threads"].GetInt();

    } else {
        std::cout << "Failed to open settings file, using defaults" << std::endl;
    }
}

void Open4X::run() {
    std::string assets_base_path = std::filesystem::current_path().string() + "/assets/glTF/";

    // NOTE:
    // GPU buffers don't exist until you allocate from them,
    // so the engine runs without any models/objects allocated,
    // then there are validation errors for missing buffers

    Model* box_model = _model_manager->get_model(assets_base_path + "Box.gltf");
    _object_manager->create_n_objects(box_model, settings->extra_object_count);

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
              << std::chrono::duration<float, std::chrono::milliseconds::period>(std::chrono::high_resolution_clock::now() - refresh_start)
                     .count()
              << "ms" << std::endl;

    auto start_time = std::chrono::high_resolution_clock::now();
    std::cout << "Total load time: " << std::chrono::duration<float, std::chrono::milliseconds::period>(start_time - creation_time).count()
              << "ms" << std::endl;

    float title_frame_time = 0.0f;
    std::chrono::time_point<std::chrono::system_clock> animation_base_time = std::chrono::high_resolution_clock::now();
    GPUAllocation* prefix_sum_buffer = gpu_allocator->get_buffer("PrefixSum");
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
        // TODO
        // cache camera matrix
        shader_globals.proj_view = cam.proj_view();
        shader_globals.cam_pos = cam.position();
        globals_alloc->write(&shader_globals);
        renderer->rg->set_push_constant("triangle_frag", "light_count", _object_manager->light_count());
        // TODO:
        // set push constant data by variable name inside the push constant
        renderer->rg->set_push_constant("frustum_data", "frustum", cam.frustum());
        renderer->rg->set_push_constant("frustum_data", "totalObjectCount", (uint32_t)_object_manager->object_count());

        // returns true when swapchain was resized
        if (renderer->render()) {
            extent = Window::window->extent();
            aspect_ratio = (float)extent.width / (float)extent.height;
            // adjust perspective projection matrix
            cam.update_projection(vertical_fov, aspect_ratio, near, far);
        }
        if (settings->show_fps) {
            std::stringstream title;
            uint32_t objects_drawn =
                *(reinterpret_cast<const uint32_t*>(prefix_sum_buffer->mapped()) + (prefix_sum_buffer->size() / sizeof(uint32_t)) - 1);
            title << "Objects: " << _object_manager->object_count() << " "
                  << "Frametime: " << std::fixed << std::setprecision(2) << (title_frame_time * 1000) << "ms"
                  << " "
                  << "Framerate: " << 1.0 / title_frame_time << " instances drawn: " << objects_drawn;
            glfwSetWindowTitle(Window::window->glfw_window(), title.str().c_str());
        }
    }
    vkDeviceWaitIdle(Device::device->vk_device());
}
