#ifndef IMAGE_H_
#define IMAGE_H_

#include "device.hpp"
#include "stb/stb_image.h"
#include <fastgltf/core.hpp>
#include <string>
#include <vulkan/vulkan_core.h>

class Image {
  public:
    Image(fastgltf::Asset const& asset, uint32_t image_ID, std::string relative_path);
    Image(std::string path);
    ~Image();
    int const& width() const { return _tex_width; }
    int const& height() const { return _tex_height; }
    int const& channels() const { return _tex_channels; }
    uint32_t const& mip_levels() const { return _mip_levels; }
    stbi_uc const* pixels() const { return _pixels; }
    VkImage const& vk_image() const { return _vk_image; }
    VkDescriptorImageInfo const& image_info() const { return _image_info; }

  private:
    void write_image(std::string path);
    bool read_image(std::string path);
    void load_pixels();
    int _tex_width, _tex_height, _tex_channels;
    uint32_t _mip_levels;
    stbi_uc* _pixels;
    bool _stbi_flag = false;
    std::vector<unsigned char> _pixel_read_buffer;
    VkImage _vk_image;
    VkImageView _vk_image_view;
    VmaAllocation _vma_allocation;
    VkDescriptorImageInfo _image_info{};
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
