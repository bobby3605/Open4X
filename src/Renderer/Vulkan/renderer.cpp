#include "renderer.hpp"
#include "../Allocator/base_allocator.hpp"
#include "descriptor_manager.hpp"
#include "device.hpp"
#include "draw.hpp"
#include "globals.hpp"
#include "memory_manager.hpp"
#include "object.hpp"
#include <cstdint>
#include <vulkan/vulkan_core.h>

Renderer::Renderer(Settings* settings) : _settings(settings) {
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
        gpu_allocator->create_buffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, "indirect_count"));

    draw_allocators.indirect_count_alloc = draw_allocators.indirect_count->alloc();
    uint32_t zero = 0;
    draw_allocators.indirect_count_alloc->write(&zero);

    draw_allocators.instance_indices = new LinearAllocator(gpu_allocator->create_buffer(
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, "InstanceIndices"));

    draw_allocators.material_data = new FixedAllocator(
        sizeof(NewMaterialData),
        gpu_allocator->create_buffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, "Materials"));

    draw_allocators.material_indices = new ContiguousFixedAllocator(
        sizeof(uint32_t),
        gpu_allocator->create_buffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, "MaterialIndices"));

    gpu_allocator->create_buffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, "CulledInstanceIndices");

    gpu_allocator->create_buffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, "CulledMaterialIndices");

    gpu_allocator->create_buffer(VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, "CulledIndirectCount");

    gpu_allocator->create_buffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, "PrefixSum");

    gpu_allocator->create_buffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, "PartialSums");

    gpu_allocator->create_buffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, "ActiveLanes");

    gpu_allocator->create_buffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, "CulledDrawCommands");

    gpu_allocator->create_buffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, "VisibilityBuffer");
}

bool Renderer::render() {
    //    gpu_allocator->get_buffer("CulledInstanceIndices")->flush_copies();
    auto match_size = [&](std::string const& read_buffer, std::string const& write_buffer) {
        size_t size = gpu_allocator->get_buffer(read_buffer)->size();
        GPUAllocation* tmp = gpu_allocator->get_buffer(write_buffer);
        if (tmp->size() < size) {
            tmp->realloc(size);
        }
    };
    match_size("InstanceIndices", "CulledInstanceIndices");
    match_size("MaterialIndices", "CulledMaterialIndices");
    // match_size("indirect_count", "CulledIndirectCount");
    match_size("InstanceIndices", "VisibilityBuffer");
    match_size("InstanceIndices", "PrefixSum");
    match_size("InstanceIndices", "PartialSums");
    match_size("InstanceIndices", "ActiveLanes");
    match_size("indirect_commands", "CulledDrawCommands");
    PtrWriter writer(rg->get_push_constant("instance_count"));
    writer.write(gpu_allocator->get_buffer("InstanceIndices")->size() / sizeof(uint32_t));
    return rg->render();
}

/*

RenderOp pushConstants(std::shared_ptr<VulkanShader> shader, void* data) {
    return [=](VkCommand_buffer command_buffer) {
        vkCmdPushConstants(command_buffer, pipelines[getFilenameNoExt(shader->name)]->pipelineLayout(), shader->stageFlags,
                           shader->pushConstantRange.offset, shader->pushConstantRange.size, data);
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


*/
void Renderer::create_rendergraph() {
    rg = new RenderGraph(_command_pool);

    std::string baseShaderPath = "assets/shaders/compute/";

    struct spec_data {
        uint32_t local_size_x;
        uint32_t subgroup_size;
    } specData;
    specData.local_size_x = Device::device->max_compute_workgroup_invocations();
    specData.subgroup_size = Device::device->max_subgroup_size();

    rg->memory_barrier(VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                       VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
    rg->compute_pass(baseShaderPath + "frustum_cull_objects.comp", &specData);

    rg->add_dynamic_node({}, [&](RenderNode& node) {
        node.op = [&](VkCommandBuffer cmd_buffer) {
            vkCmdDispatch(cmd_buffer,
                          get_group_count(gpu_allocator->get_buffer("ObjectCullingData")->size() / sizeof(ObjectCullData),
                                          Device::device->max_compute_workgroup_invocations()),
                          1, 1);
        };
    });
    rg->memory_barrier(VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                       VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);

    // ensure previous frame vertex read completed before writing
    rg->memory_barrier(VK_ACCESS_2_SHADER_STORAGE_READ_BIT, VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
                       VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
    rg->compute_pass(baseShaderPath + "cull_visibility_buffer.comp", &specData);
    // TODO:
    // Put this into compute_pass
    rg->add_dynamic_node({}, [&](RenderNode& node) {
        node.op = [&](VkCommandBuffer cmd_buffer) {
            vkCmdDispatch(cmd_buffer,
                          get_group_count(gpu_allocator->get_buffer("InstanceIndices")->size() / sizeof(uint32_t),
                                          Device::device->max_compute_workgroup_invocations()),
                          1, 1);
        };
    });

    // wait until the frustum culling is done
    rg->memory_barrier(VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
                       VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);

    rg->compute_pass(baseShaderPath + "reduce_prefix_sum.comp", &specData);

    rg->add_dynamic_node({}, [&](RenderNode& node) {
        node.op = [&](VkCommandBuffer cmd_buffer) {
            vkCmdDispatch(cmd_buffer,
                          get_group_count(gpu_allocator->get_buffer("InstanceIndices")->size() / sizeof(uint32_t),
                                          Device::device->max_compute_workgroup_invocations()),
                          1, 1);
        };
    });

    // wait until finished
    rg->memory_barrier(VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
                       VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);

    // ensure previous frame vertex read completed before zeroing out buffer
    rg->memory_barrier(VK_ACCESS_2_SHADER_STORAGE_READ_BIT, VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
                       VK_PIPELINE_STAGE_2_COPY_BIT);

    //    rg->set_buffer("CulledDrawIndirectCount", 0);
    gpu_allocator->get_buffer("CulledIndirectCount")->realloc(sizeof(uint32_t));
    rg->add_node(vkCmdFillBuffer, gpu_allocator->get_buffer("CulledIndirectCount")->buffer(), 0, VK_WHOLE_SIZE, 0);

    // barrier until the CulledDrawIndirectCount buffer has been cleared
    rg->memory_barrier(VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_COPY_BIT,
                       VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);

    rg->compute_pass(baseShaderPath + "cull_draws.comp", &specData);
    rg->add_dynamic_node({}, [&](RenderNode& node) {
        node.op = [&](VkCommandBuffer cmd_buffer) {
            vkCmdDispatch(cmd_buffer,
                          get_group_count(gpu_allocator->get_buffer("indirect_commands")->size() / sizeof(VkDrawIndexedIndirectCommand),
                                          Device::device->max_compute_workgroup_invocations()),
                          1, 1);
        };
    });

    // wait until culling is completed
    rg->memory_barrier(VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                       VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_READ_BIT, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT);
    rg->memory_barrier(VK_ACCESS_2_SHADER_STORAGE_READ_BIT, VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
                       VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);

    baseShaderPath = "assets/shaders/graphics/";
    std::string baseShaderName = "triangle";

    std::string vert_shader_path = baseShaderPath + baseShaderName + ".vert";
    std::string frag_shader_path = baseShaderPath + baseShaderName + ".frag";

    rg->begin_rendering();

    rg->graphics_pass(vert_shader_path, frag_shader_path, draw_allocators.vertex, draw_allocators.index);

    rg->add_dynamic_node({}, [&](RenderNode& node) {
        node.op = [&](VkCommandBuffer cmd_buffer) {
            vkCmdDrawIndexedIndirectCount(cmd_buffer, gpu_allocator->get_buffer("CulledDrawCommands")->buffer(), 0,
                                          gpu_allocator->get_buffer("CulledIndirectCount")->buffer(), 0,
                                          draw_allocators.indirect_commands->block_count(),
                                          draw_allocators.indirect_commands->block_size());
        };
    });

    rg->end_rendering();
}
