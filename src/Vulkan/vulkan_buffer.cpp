#include "vulkan_buffer.hpp"
#include "common.hpp"
#include "vulkan_device.hpp"
#include "vulkan_image.hpp"
#include <cstring>
#include <glm/fwd.hpp>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

VulkanBuffer::VulkanBuffer(VulkanDevice* device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
    : device{device} {
    device->createBuffer(size, usage, properties, buffer(), memory());
    _bufferInfo.range = size;
    _bufferInfo.offset = 0;
    _usageFlags = usage;
}

void VulkanBuffer::map() {
    checkResult(vkMapMemory(device->device(), memory(), 0, size(), 0, &_mapped), "memory map failed");
    isMapped = true;
}

void VulkanBuffer::unmap() {
    vkUnmapMemory(device->device(), memory());
    isMapped = false;
}

void VulkanBuffer::write(void* data, VkDeviceSize size, VkDeviceSize offset) { memcpy(((char*)mapped()) + offset, data, size); }

void VulkanBuffer::write(void* data) { write(data, size(), _bufferInfo.offset); }

VulkanBuffer::~VulkanBuffer() {
    if (isMapped)
        unmap();
    vkDestroyBuffer(device->device(), buffer(), nullptr);
    vkFreeMemory(device->device(), memory(), nullptr);
}

std::shared_ptr<VulkanBuffer> VulkanBuffer::StagedBuffer(VulkanDevice* device, void* data, VkDeviceSize size,
                                                         VkBufferUsageFlags usageFlags) {

    VulkanBuffer stagingBuffer(device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    stagingBuffer.map();
    stagingBuffer.write(data, size, 0);
    stagingBuffer.unmap();

    std::shared_ptr<VulkanBuffer> stagedBuffer =
        std::make_shared<VulkanBuffer>(device, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    device->singleTimeCommands().copyBuffer(stagingBuffer.buffer(), stagedBuffer->buffer(), size).run();

    return stagedBuffer;
}

std::shared_ptr<VulkanBuffer> VulkanBuffer::UniformBuffer(VulkanDevice* device, VkDeviceSize size) {

    std::shared_ptr<VulkanBuffer> uniformBuffer = std::make_shared<VulkanBuffer>(
        device, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    uniformBuffer->map();

    return uniformBuffer;
}

std::shared_ptr<VulkanBuffer> VulkanBuffer::StorageBuffer(VulkanDevice* device, VkDeviceSize size, VkMemoryPropertyFlags memoryFlags) {
    std::shared_ptr<VulkanBuffer> storageBuffer =
        std::make_shared<VulkanBuffer>(device, size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, memoryFlags);

    if (memoryFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        storageBuffer->map();
    }

    return storageBuffer;
}

SSBOBuffers::SSBOBuffers(VulkanDevice* device) : device{device} {}

void SSBOBuffers::createMaterialBuffer(uint32_t drawsCount) {
    // NOTE:
    // could be optimized further by only using referenced materials
    _materialBuffer = VulkanBuffer::StorageBuffer(device, (drawsCount + 1) * sizeof(MaterialData));
    materialMapped = reinterpret_cast<MaterialData*>(_materialBuffer->mapped());
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
    _ssboBuffer = VulkanBuffer::StorageBuffer(device, instanceCount * sizeof(SSBOData));
    ssboMapped = reinterpret_cast<SSBOData*>(_ssboBuffer->mapped());
    _instanceIndicesBuffer = VulkanBuffer::StorageBuffer(device, instanceCount * sizeof(uint32_t));
    _materialIndicesBuffer = VulkanBuffer::StorageBuffer(device, instanceCount * sizeof(uint32_t));
    _culledMaterialIndicesBuffer = VulkanBuffer::StorageBuffer(device, instanceCount * sizeof(uint32_t));

    instanceIndicesMapped = reinterpret_cast<uint32_t*>(_instanceIndicesBuffer->mapped());
    materialIndicesMapped = reinterpret_cast<uint32_t*>(_materialIndicesBuffer->mapped());
}
