#ifndef IMAGE_H_
#define IMAGE_H_

#include "model.hpp"
#include "stb/stb_image.h"
#include <string>
#include <vulkan/vulkan_core.h>

class Image {
  public:
    VkDescriptorImageInfo const& image_info() const { return _image_info; }
    VkDescriptorImageInfo _image_info{};
    Image(Model* model, uint32_t textureID, VkFormat format);
    Image(std::string path, VkFormat format);

  private:
    void write_image(std::string path);
    bool read_image(std::string path);
    void load_pixels();
    uint32_t tex_width, tex_height, tex_channels;
    stbi_uc* _pixels;
    std::vector<unsigned char> _pixel_read_buffer;
};

class Sampler {};

#endif // IMAGE_H_
