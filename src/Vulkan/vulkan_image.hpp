#ifndef VULKAN_IMAGE_H_
#define VULKAN_IMAGE_H_
#include "../../external/stb_image.h"
#include "../glTF/GLTF.hpp"
#include "vulkan_device.hpp"
#include <cstdint>
#include <string>
#include <vulkan/vulkan.hpp>

class VulkanImage {
  public:
    VulkanImage(VulkanDevice* device, GLTF* model, uint32_t textureID);
    VulkanImage(VulkanDevice* device, std::string path);
    ~VulkanImage();

    VkWriteDescriptorSet descriptorWrite{};
    VkImageView const imageView() { return _imageView; }
    VkImage const image() { return _image; }
    VkDeviceMemory const imageMemory() { return _imageMemory; }
    uint32_t const mipLevels() { return _mipLevels; }
    uint32_t const textureID() { return _textureID; }
    VkDescriptorImageInfo imageInfo{};

  private:
    void loadPixels();
    stbi_uc* pixels;

    VulkanDevice* device;
    GLTF* model = nullptr;
    uint32_t _textureID;

    int texWidth, texHeight, texChannels;
    VkImageView _imageView;
    VkImage _image;
    VkDeviceMemory _imageMemory;
    uint32_t _mipLevels;
};

class VulkanSampler {
  public:
    VulkanSampler(VulkanDevice* device, GLTF* model, uint32_t samplerID, uint32_t mipLevels);
    ~VulkanSampler();
    VkSampler const imageSampler() { return _imageSampler; }
    friend bool operator==(const VulkanSampler& s1, const VulkanSampler& s2);

  private:
    VulkanDevice* device;
    GLTF* model;
    uint32_t samplerID, mipLevels;
    VkSampler _imageSampler;
};

#endif // VULKAN_IMAGE_H_
