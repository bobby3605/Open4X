#ifndef IMAGE_H_
#define IMAGE_H_

#include "device.hpp"
#include "stb/stb_image.h"
#include <fastgltf/core.hpp>
#include <string>
#include <vulkan/vulkan_core.h>

class Image {
  public:
    /*
    VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
    VkSampleCountFlagBits num_samples = VK_SAMPLE_COUNT_1_BIT;
    VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
    VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    VkImageAspectFlags aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT;
    */
    Image(VkFormat format, VkSampleCountFlagBits num_samples, VkImageTiling tiling, VkImageUsageFlags usage,
          VkMemoryPropertyFlags mem_props, VkImageAspectFlags image_aspect, uint32_t width, uint32_t height, uint32_t mip_levels);
    ~Image();
    void transition(VkImageLayout new_layout);
    void load_pixels(const void* src, VkImageLayout final_layout);
    VkDescriptorImageInfo const& image_info() const { return _image_info; };
    VkImage const& vk_image() const { return _vk_image; }

  private:
    VkImage _vk_image;
    VmaAllocation _vma_allocation;
    VkDescriptorImageInfo _image_info{};
    uint32_t _width;
    uint32_t _height;
    VkFormat _format;
    uint32_t _mip_levels;
};

class Texture {
  public:
    Texture(fastgltf::Asset const& asset, uint32_t image_ID, std::string relative_path);
    Texture(std::string path);
    ~Texture();
    int const& width() const { return _tex_width; }
    int const& height() const { return _tex_height; }
    int const& channels() const { return _tex_channels; }
    stbi_uc const* pixels() const { return _pixels; }
    VkDescriptorImageInfo const& image_info() const { return _image->image_info(); }

  private:
    void write_image(std::string path);
    bool read_image(std::string path);
    void load_pixels();
    int _tex_width, _tex_height, _tex_channels;
    stbi_uc* _pixels;
    bool _stbi_flag = false;
    std::vector<unsigned char> _pixel_read_buffer;
    Image* _image = nullptr;
};

class Sampler {
  public:
    Sampler(fastgltf::Asset const& asset, uint32_t sampler_ID, uint32_t mip_levels);
    Sampler(uint32_t mip_levels);
    ~Sampler();
    VkSampler const& vk_sampler() { return _vk_sampler; }
    friend bool operator==(const Sampler& s1, const Sampler& s2);
    VkDescriptorImageInfo const& image_info() const { return _image_info; }

  private:
    uint32_t _sampler_ID, _mip_levels;
    VkSampler _vk_sampler;
    VkDescriptorImageInfo _image_info{};
};

#endif // IMAGE_H_
