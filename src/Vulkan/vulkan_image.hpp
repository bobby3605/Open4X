#ifndef VULKAN_IMAGE_H_
#define VULKAN_IMAGE_H_
#include "../glTF/GLTF.hpp"
#include "vulkan_device.hpp"
#include <string>
#include <vulkan/vulkan.hpp>

class VulkanImage {
  public:
    VulkanImage(VulkanDevice* device, GLTF* model, uint32_t textureID);
    ~VulkanImage();

    VkWriteDescriptorSet descriptorWrite{};
    VkSampler const imageSampler() { return _imageSampler; }
    VkImageView const imageView() { return _imageView; }
    VkImage const image() { return _image; }
    VkDeviceMemory const imageMemory() { return _imageMemory; }
    uint32_t const mipLevels() { return _mipLevels; }

  private:
    VulkanDevice* device;

    VkSampler _imageSampler;
    VkImageView _imageView;
    VkImage _image;
    VkDeviceMemory _imageMemory;
    uint32_t _mipLevels;
};
#endif // VULKAN_IMAGE_H_
