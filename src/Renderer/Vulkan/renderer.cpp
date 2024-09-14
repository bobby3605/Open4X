#include "renderer.hpp"
#include "../Allocator/base_allocator.hpp"
#include "descriptor_manager.hpp"
#include "device.hpp"
#include "draw.hpp"
#include "globals.hpp"
#include "memory_manager.hpp"
#include <cstdint>
#include <vulkan/vulkan_core.h>

Renderer::Renderer(NewSettings* settings) : _settings(settings) {
    new Device();
    globals.device = Device::device;
    new MemoryManager();
    globals.memory_manager = MemoryManager::memory_manager;
    gpu_allocator = new GPUAllocator();
    new DescriptorManager(gpu_allocator);
    create_data_buffers();
    _command_pool = Device::device->command_pools()->get_pool();
    create_rendergraph();
}

Renderer::~Renderer() {
    Device::device->command_pools()->release_pool(_command_pool);
    vkDeviceWaitIdle(Device::device->vk_device());
    delete rg;
    gpu_allocator->get_buffer("CulledInstanceIndices")->flush_copies();
    delete gpu_allocator;
    delete MemoryManager::memory_manager;
    delete Device::device;
}

void Renderer::create_data_buffers() {
    draw_allocators.vertex = new LinearAllocator(gpu_allocator->create_buffer(
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, "vertex_buffer"));

    draw_allocators.index = new LinearAllocator(gpu_allocator->create_buffer(
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, "index_buffer"));

    // TODO:
    // Have some kind of interface mapping for shader buffer names to required buffers as metadata in the shader
    draw_allocators.instance_data = new FixedAllocator(
        sizeof(InstanceData),
        gpu_allocator->create_buffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, "Instances"));

    draw_allocators.indirect_commands = new ContiguousFixedAllocator(
        sizeof(VkDrawIndexedIndirectCommand),
        gpu_allocator->create_buffer(VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, "indirect_commands"));

    draw_allocators.indirect_count = new FixedAllocator(
        sizeof(uint32_t),
        gpu_allocator->create_buffer(VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, "indirect_count"));

    draw_allocators.indirect_count_alloc = draw_allocators.indirect_count->alloc();

    draw_allocators.instance_indices = new LinearAllocator(
        gpu_allocator->create_buffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, "CulledInstanceIndices"));

    draw_allocators.material_data = new FixedAllocator(
        sizeof(NewMaterialData),
        gpu_allocator->create_buffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, "Materials"));

    draw_allocators.material_indices = new ContiguousFixedAllocator(
        sizeof(uint32_t),
        gpu_allocator->create_buffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, "CulledMaterialIndices"));
}

bool Renderer::render() {
    //    gpu_allocator->get_buffer("CulledInstanceIndices")->flush_copies();
    return rg->render();
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

    rg->begin_rendering();

    std::string baseShaderPath = "assets/shaders/graphics/";
    std::string baseShaderName = "triangle";

    std::string vert_shader_path = baseShaderPath + baseShaderName + ".vert";
    std::string frag_shader_path = baseShaderPath + baseShaderName + ".frag";

    rg->graphics_pass(vert_shader_path, frag_shader_path, draw_allocators.vertex, draw_allocators.index);

    rg->add_dynamic_node({}, [&](RenderNode& node) {
        node.op = [&](VkCommandBuffer cmd_buffer) {
            vkCmdDrawIndexedIndirectCount(
                cmd_buffer, draw_allocators.indirect_commands->parent()->buffer(), 0, draw_allocators.indirect_count->parent()->buffer(), 0,
                draw_allocators.indirect_commands->block_count(), draw_allocators.indirect_commands->block_size());
        };
    });

    rg->end_rendering();
}
