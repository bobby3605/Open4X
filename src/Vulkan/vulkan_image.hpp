#ifndef VULKAN_IMAGE_H_
#define VULKAN_IMAGE_H_
#include "../glTF/GLTF.hpp"
#include "stb/stb_image.h"
#include "vulkan_device.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <vulkan/vulkan.hpp>

class VulkanImage {
  public:
    VulkanImage(std::shared_ptr<VulkanDevice> device, GLTF* model, uint32_t textureID, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB);
    VulkanImage(std::shared_ptr<VulkanDevice> device, std::string path, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB);
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
    void writeImage(std::string path);
    bool readImage(std::string path);
    stbi_uc* pixels;
    std::vector<unsigned char> pixelBuffer;

    std::shared_ptr<VulkanDevice> device;
    GLTF* model = nullptr;
    uint32_t _textureID;

    int texWidth, texHeight, texChannels;
    VkImageView _imageView;
    VkImage _image;
    VkDeviceMemory _imageMemory;
    uint32_t _mipLevels;

    VkFormat _format;

    bool stbiFlag = 0;
};

class VulkanSampler {
  public:
    VulkanSampler(std::shared_ptr<VulkanDevice> device, GLTF* model, uint32_t samplerID, uint32_t mipLevels);
    VulkanSampler(std::shared_ptr<VulkanDevice> device, uint32_t mipLevels);
    ~VulkanSampler();
    VkSampler const imageSampler() { return _imageSampler; }
    friend bool operator==(const VulkanSampler& s1, const VulkanSampler& s2);

  private:
    std::shared_ptr<VulkanDevice> device;
    GLTF* model;
    uint32_t samplerID, mipLevels;
    VkSampler _imageSampler;
};

#endif // VULKAN_IMAGE_H_
