#include "vulkan_buffer.hpp"
#include "common.hpp"
#include "vulkan_device.hpp"
#include "vulkan_image.hpp"
#include <cstring>
#include <glm/fwd.hpp>
#include <iostream>
#include <vulkan/vulkan_core.h>

VulkanBuffer::VulkanBuffer(VulkanDevice* device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
    : device{device}, bufferSize{size} {
    device->createBuffer(size, usage, properties, buffer, memory);
}

void VulkanBuffer::map() {
    checkResult(vkMapMemory(device->device(), memory, 0, bufferSize, 0, &mapped), "memory map failed");
    isMapped = true;
}

void VulkanBuffer::unmap() {
    vkUnmapMemory(device->device(), memory);
    isMapped = false;
}

void VulkanBuffer::write(void* data, VkDeviceSize size, VkDeviceSize offset) { memcpy(((char*)mapped) + offset, data, size); }

VulkanBuffer::~VulkanBuffer() {
    if (isMapped)
        unmap();
    vkDestroyBuffer(device->device(), buffer, nullptr);
    vkFreeMemory(device->device(), memory, nullptr);
}

StagedBuffer::StagedBuffer(VulkanDevice* device, void* data, VkDeviceSize size, VkMemoryPropertyFlags type) {

    VulkanBuffer stagingBuffer(device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    stagingBuffer.map();
    stagingBuffer.write(data, size, 0);
    stagingBuffer.unmap();

    stagedBuffer = new VulkanBuffer(device, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | type, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    device->singleTimeCommands().copyBuffer(stagingBuffer.buffer, stagedBuffer->buffer, size).run();
}

StagedBuffer::~StagedBuffer() { delete stagedBuffer; }

UniformBuffer::UniformBuffer(VulkanDevice* device, VkDeviceSize size) {

    uniformBuffer = new VulkanBuffer(device, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    uniformBuffer->map();

    bufferInfo.buffer = uniformBuffer->buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = size;
}

void* UniformBuffer::mapped() { return uniformBuffer->getMapped(); }

UniformBuffer::~UniformBuffer() {
    uniformBuffer->unmap();
    delete uniformBuffer;
}

void UniformBuffer::write(void* data) { uniformBuffer->write(data, sizeof(UniformBufferObject), 0); }

StorageBuffer::StorageBuffer(VulkanDevice* device, VkDeviceSize size) {
    storageBuffer = new VulkanBuffer(device, size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    storageBuffer->map();
    mapped = storageBuffer->getMapped();
}

StorageBuffer::~StorageBuffer() { delete storageBuffer; }

SSBOBuffers::SSBOBuffers(VulkanDevice* device, uint32_t drawsCount) : device{device} {
    // NOTE:
    // could be optimized further by only using referenced materials
    _materialBuffer = new StorageBuffer(device, (drawsCount + 1) * sizeof(MaterialData));
    materialMapped = reinterpret_cast<MaterialData*>(_materialBuffer->mapped);
    // create default material at index 0
    MaterialData materialData{};
    materialData.baseColorFactor = {1.0f, 1.0f, 1.0f, 1.0f};
    materialData.samplerIndex = 0;
    materialData.normalMapIndex = 0;
    materialData.imageIndex = 0;
    materialMapped[0] = materialData;
}

void SSBOBuffers::createInstanceBuffers(uint32_t instanceCount) {
    // NOTE:
    // must only be called once after uniqueObjectID has been set
    _ssboBuffer = new StorageBuffer(device, instanceCount * sizeof(SSBOData));
    ssboMapped = reinterpret_cast<SSBOData*>(_ssboBuffer->mapped);
    _instanceIndicesBuffer = new StorageBuffer(device, instanceCount * sizeof(uint32_t));
    _materialIndicesBuffer = new StorageBuffer(device, instanceCount * sizeof(uint32_t));
    _culledMaterialIndicesBuffer = new StorageBuffer(device, instanceCount * sizeof(uint32_t));

    instanceIndicesMapped = reinterpret_cast<uint32_t*>(_instanceIndicesBuffer->mapped);
    materialIndicesMapped = reinterpret_cast<uint32_t*>(_materialIndicesBuffer->mapped);
}

SSBOBuffers::~SSBOBuffers() {
    delete _ssboBuffer;
    delete _materialBuffer;
    delete _instanceIndicesBuffer;
    delete _materialIndicesBuffer;
    delete _culledMaterialIndicesBuffer;
}
