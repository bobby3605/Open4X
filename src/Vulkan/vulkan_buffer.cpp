#include "vulkan_buffer.hpp"
#include "common.hpp"
#include "vulkan_device.hpp"
#include <cstring>
#include <iostream>

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

UniformBuffer::UniformBuffer(VulkanDevice* device) {

    uniformBuffer = new VulkanBuffer(device, sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    uniformBuffer->map();

    bufferInfo.buffer = uniformBuffer->buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);
}

UniformBuffer::~UniformBuffer() { delete uniformBuffer; }

void UniformBuffer::write(void* data) { uniformBuffer->write(data, sizeof(UniformBufferObject), 0); }

StorageBuffer::StorageBuffer(VulkanDevice* device, VkDeviceSize size) {
    // TODO
    // Dynamically resize this buffer
    // VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    // is only 256MB on GPUs without resizable bar support
    storageBuffer = new VulkanBuffer(device, size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    storageBuffer->map();
    mapped = storageBuffer->getMapped();
}

StorageBuffer::~StorageBuffer() { delete storageBuffer; }

SSBOBuffers::SSBOBuffers(VulkanDevice* device, uint32_t count) {
    _ssboBuffer = new StorageBuffer(device, count * sizeof(SSBOData));
    _materialBuffer = new StorageBuffer(device, count * sizeof(MaterialData));
    ssboMapped = reinterpret_cast<SSBOData*>(_ssboBuffer->mapped);
    materialMapped = reinterpret_cast<MaterialData*>(_materialBuffer->mapped);
}

SSBOBuffers::~SSBOBuffers() {
    delete _ssboBuffer;
    delete _materialBuffer;
}
