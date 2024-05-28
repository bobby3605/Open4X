#include "renderer.hpp"
#include "device.hpp"
#include "memory_manager.hpp"
#include "model.hpp"
#include "object_manager.hpp"
#include "swapchain.hpp"
#include "window.hpp"
#include <cstdint>
#include <vulkan/vulkan_core.h>

Renderer::Renderer(NewSettings* settings) : _settings(settings) {
    new Device();
    new MemoryManager();
    create_data_buffers();
    _swap_chain = new SwapChain(Window::window->extent());
    _command_pool = Device::device->command_pools()->get_pool();

    create_rendergraph();
}

Renderer::~Renderer() {
    Device::device->command_pools()->release_pool(_command_pool);
    vkDeviceWaitIdle(Device::device->vk_device());
    delete rg;
    delete _swap_chain;
    delete MemoryManager::memory_manager;
    delete Device::device;
}

void Renderer::create_data_buffers() {
    Buffer* vertex_buffer = MemoryManager::memory_manager->create_buffer(
        "vertex_buffer", sizeof(NewVertex),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    Buffer* index_buffer = MemoryManager::memory_manager->create_buffer(
        "index_buffer", sizeof(uint32_t),
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    /*
    Buffer* instance_data_buffer = MemoryManager::memory_manager->create_buffer(
        "Instances", sizeof(InstanceData),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        */

    instance_data_allocator =
        new GPUAllocator(sizeof(InstanceData),
                         VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, "Instances");

    instances_stack_allocator = new StackAllocator<GPUAllocator>(sizeof(InstanceData), instance_data_allocator);

    MemoryManager::memory_manager->create_buffer("indirect_commands", sizeof(VkDrawIndexedIndirectCommand),
                                                 VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    MemoryManager::memory_manager->create_buffer("indirect_count", sizeof(uint32_t),
                                                 VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    Buffer* culled_instance_indices_buffer = MemoryManager::memory_manager->create_buffer(
        "CulledInstanceIndices", sizeof(uint32_t),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    Buffer* culled_material_indices_buffer = MemoryManager::memory_manager->create_buffer(
        "CulledMaterialIndices", sizeof(uint32_t),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    /*
    ObjectManager* manager;
    Object* box = manager->get_object("box");
    uint32_t instance_index;
    // This isn't right
    // gl_InstanceIndex will be different
    // Transposed with the primitives or something like that
    culled_instance_indices_buffer->data()[instance_index] = box->instance_data_offset();
    */
}

void Renderer::recreate_swap_chain() {
    // pause on minimization
    if (_settings->pauseOnMinimization) {
        int width = 0, height = 0;
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(Window::window->glfw_window(), &width, &height);
            glfwWaitEvents();
        }
    }

    vkDeviceWaitIdle(Device::device->vk_device());

    // FIXME:
    // recreate swap chain
    //    swapChain = new VulkanSwapChain(_device, _window->getExtent(), swapChain);
}

bool Renderer::render() {
    // Start Frame
    VkResult result = _swap_chain->acquire_next_image();
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        // TODO
        // Do I need to run acquireNextImage after this?
        recreate_swap_chain();
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image");
    }
    // Submit Frame
    //    result = _swap_chain->submit_command_buffers(rg->command_buffer());
    result = rg->submit(_swap_chain);
    // End Frame
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        recreate_swap_chain();
        // return true if framebuffer was resized
        return true;
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image");
    }
    return false;
}

/*

RenderOp pushConstants(std::shared_ptr<VulkanShader> shader, void* data) {
    return [=](VkCommand_buffer command_buffer) {
        vkCmdPushConstants(command_buffer, pipelines[getFilenameNoExt(shader->name)]->pipelineLayout(), shader->stageFlags,
                           shader->pushConstantRange.offset, shader->pushConstantRange.size, data);
    };
}

RenderOp fill_buffer(std::string buffer_name, VkDeviceSize offset, VkDeviceSize size, uint32_t value) {
    return [=](VkCommandBuffer command_buffer) {
        vkCmdFillBuffer(command_buffer, MemoryManager::memory_manager->get_buffer(buffer_name)->vk_buffer(), offset, size, value);
    };
}

RenderOp dispatch(uint32_t group_count_X, uint32_t group_count_Y, uint32_t group_count_Z) {
    return [=](VkCommandBuffer command_buffer) { vkCmdDispatch(command_buffer, group_count_X, group_count_Y, group_count_Z); };
}

RenderOp write_timestamp(VkPipelineStageFlags2 stage_flags, VkQueryPool query_pool, uint32_t query) {
    return [=](VkCommandBuffer command_buffer) { vkCmdWriteTimestamp2(command_buffer, stage_flags, query_pool, query); };
}

RenderOp reset_query_pool(VkQueryPool query_pool, uint32_t first_query, uint32_t count) {
    return [=](VkCommandBuffer command_buffer) { vkCmdResetQueryPool(command_buffer, query_pool, first_query, count); };
}


RenderOp memory_barrier(VkAccessFlags2 src_access_mask, VkPipelineStageFlags2 src_stage_mask, VkAccessFlags2 dst_access_mask,
                        VkPipelineStageFlags2 dst_stage_mask) {
    return [=](VkCommandBuffer command_buffer) {
        VkMemoryBarrier2 memory_barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER_2};
        memory_barrier.pNext = VK_NULL_HANDLE;
        memory_barrier.srcAccessMask = src_access_mask;
        memory_barrier.srcStageMask = src_stage_mask;
        memory_barrier.dstAccessMask = dst_access_mask;
        memory_barrier.dstStageMask = dst_stage_mask;

        VkDependencyInfo dependency_info{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
        dependency_info.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        dependency_info.memoryBarrierCount = 1;
        dependency_info.pMemoryBarriers = &memory_barrier;

        vkCmdPipelineBarrier2(command_buffer, &dependency_info);
    };
}
*/

void Renderer::create_rendergraph() {
    rg = new RenderGraph(_command_pool);
    rg->buffer("Globals", 1);
    rg->buffer("Materials", 1);
    rg->buffer("CulledInstanceIndices", 1);
    rg->buffer("CulledMaterialIndices", 1);

    rg->define_primary(false);
    // FIXME:
    // rg should hold the swap chain
    rg->begin_rendering(_swap_chain);

    std::string baseShaderPath = "assets/shaders/graphics/";
    std::string baseShaderName = "triangle";

    std::string vert_shader_path = baseShaderPath + baseShaderName + ".vert";
    std::string frag_shader_path = baseShaderPath + baseShaderName + ".frag";

    rg->graphics_pass(vert_shader_path, frag_shader_path, "vertex_buffer", "index_buffer", _swap_chain);

    VkExtent2D extent = _swap_chain->extent();

    viewport.x = 0.0f;
    viewport.y = extent.height;
    viewport.width = extent.width;
    viewport.height = extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    scissor.offset = {0, 0};
    scissor.extent = extent;

    rg->add_node(vkCmdSetViewport, 0, 1, &viewport);
    rg->add_node(vkCmdSetScissor, 0, 1, &scissor);

    std::vector<VkDrawIndexedIndirectCommand> indirect_draws;

    rg->add_node(vkCmdDrawIndexedIndirectCount, MemoryManager::memory_manager->get_buffer("indirect_commands")->vk_buffer(), 0,
                 MemoryManager::memory_manager->get_buffer("indirect_count")->vk_buffer(), 0, indirect_draws.size(),
                 sizeof(indirect_draws[0]));

    rg->end_rendering(_swap_chain);

    rg->compile();
}
