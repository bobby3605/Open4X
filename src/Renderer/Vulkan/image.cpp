#include "image.hpp"
#include "common.hpp"
#include <fcntl.h>
#include <filesystem>
#include <stdexcept>

VkSamplerAddressMode switch_wrap(uint32_t wrap) {
    switch (wrap) {
    case 33071:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    case 33648:
        return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    case 10497:
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    default:
        throw std::runtime_error("failed to get sampler mode: " + std::to_string(wrap));
    }
}

VkFilter switch_filter(uint32_t filter) {
    switch (filter) {
    case 9728:
        return VK_FILTER_NEAREST;
    case 9729:
        return VK_FILTER_LINEAR;
        // mipmap modes
    case 9984:
        return VK_FILTER_NEAREST;
    case 9985:
        return VK_FILTER_LINEAR;
    case 9986:
        return VK_FILTER_NEAREST;
    case 9987:
        return VK_FILTER_LINEAR;
    default:
        throw std::runtime_error("failed to get sampler filter: " + std::to_string(filter));
    }
}

VkSamplerMipmapMode switch_mipmap_filter(uint32_t filter) {
    switch (filter) {
    case 9984:
        return VK_SAMPLER_MIPMAP_MODE_NEAREST;
    case 9985:
        return VK_SAMPLER_MIPMAP_MODE_NEAREST;
    case 9986:
        return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    case 9987:
        return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    default:
        throw std::runtime_error("failed to get sampler filter: " + std::to_string(filter));
    }
}

void Image::write_image(std::string path) {
    // get the writing directory and fileName of an image cache file
    // Example:
    // path: ABeautifulGame/ABeautifulGame-gltf/bishop_white_normal.jpg
    // directory: assets/cache/images/ABeautifulGame/ABeautifulGame-gltf/
    // file_name: bishop_white_normal.jpg.imageCache
    std::string directory = "assets/cache/images/" + path.substr(0, path.find_last_of("/")) + "/";
    // create the directory if it doesn't exist
    std::filesystem::create_directories(directory);
    // get the file name
    std::string file_name = path.substr(path.find_last_of("/") + 1) + ".imageCache";
    // write image data to file
    MMIO mmio = MMIO(directory + file_name, O_RDWR | O_CREAT | O_TRUNC,
                     sizeof(tex_width) + sizeof(tex_height) + sizeof(tex_channels) + tex_width * tex_height * 4);
    if (!mmio.is_open()) {
        std::cout << "WARNING: "
                  << "failed to write image cache: " << directory + file_name << std::endl;
    }
    mmio.write(tex_width);
    mmio.write(tex_height);
    mmio.write(tex_channels);
    mmio.write(_pixels, tex_width * tex_height * 4);
}

bool Image::read_image(std::string path) {
    MMIO mmio = MMIO("assets/cache/images/" + path + std::string(".imageCache"), O_RDONLY);
    if (!mmio.is_open()) {
        return false;
    }
    mmio.read(tex_width);
    mmio.read(tex_height);
    mmio.read(tex_channels);
    // NOTE:
    // _pixel_read_buffer should never be changed after this point,
    // if it does, then pixels should get a new pointer,
    // since vector operations can change the base memory location
    _pixel_read_buffer.reserve(tex_width * tex_height * 4);
    _pixels = _pixel_read_buffer.data();
    mmio.read(_pixels, tex_width * tex_height * 4);
    return true;
}

Image::Image(Model* model, uint32_t textureID, VkFormat format) : model{model}, _textureID{textureID}, _format{format} {
    uint32_t sourceID = model->textures[textureID].source;

    // Get the directory path for the image cache
    // Example:
    // model-path(): assets/glTF/ABeautifulGame/
    // model->fileName(): ABeautifulGame.gltf
    // gltfPath: glTF/ABeautifulGame/
    // path: ABeautifulGame/
    // cacheFilePath: ABeautifulGame/ABeautifulGame-gltf/

    std::string gltf_path = model->path().substr(model->path().find_first_of("/") + 1);
    std::string path = gltf_path.substr(gltf_path.find_first_of("/") + 1);
    std::string cache_file_path = path + model->file_name().substr(0, model->file_name().find_last_of(".")) + "-" +
                                  model->file_name().substr(model->file_name().find_last_of(".") + 1) + "/";

    bool stbi_flag = false;
    // TODO
    // check if file has already been loaded
    if (model->images[sourceID].uri.has_value()) {
        // add the uri to the directory path
        cache_file_path += model->images[sourceID].uri.value();
        // check if the image file has been loaded from the cache
        if (!read_image(cache_file_path)) {
            // if it hasn't been loaded,
            // load it from the gltf image and cache it
            _pixels = stbi_load((model->path() + model->images[sourceID].uri.value()).c_str(), &tex_width, &tex_height, &tex_channels,
                                STBI_rgb_alpha);
            if (!_pixels) {
                throw std::runtime_error("failed to load texture image: " + model->path() + model->images[sourceID].uri.value());
            }
            stbi_flag = true;
            write_image(cache_file_path);
        }
    } else if (model->images[sourceID].bufferView.has_value()) {
        // add bufferView-n to the directory path
        cache_file_path += "bufferView-" + std::to_string(model->images[sourceID].bufferView.value());
        if (!read_image(cache_file_path)) {
            GLTF::BufferView* bufferView = &model->bufferViews[model->images[sourceID].bufferView.value()];
            pixels = stbi_load_from_memory(model->buffers[bufferView->buffer].data.data() + bufferView->byteOffset, bufferView->byteLength,
                                           &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);
            if (!pixels) {
                throw std::runtime_error("failed to load texture image from bufferView: " +
                                         std::to_string(model->images[sourceID].bufferView.value()));
            }
            stbi_flag = true;
            write_image(cache_file_path);
        }
    } else {
        throw std::runtime_error("no data found for image on textureID: " + std::to_string(textureID));
    }

    loadPixels();
    if (stbi_flag) {
        stbi_image_free(pixels);
    }
}

Image::Image(std::string path, VkFormat format) : _format{format} {
    pixels = stbi_load(path.c_str(), &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);
    if (!pixels) {
        throw std::runtime_error("failed to load texture image");
    }
    load_pixels();
}

void Image::load_pixels() {

    VkDeviceSize image_size = tex_width * tex_height * 4;
    _mip_levels = static_cast<uint32_t>(std::floor(std::log2(std::max(tex_width, tex_height)))) + 1;

    VulkanBuffer staging_buffer(device, image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    stagingBuffer.map();
    stagingBuffer.write(pixels, image_size);
    stagingBuffer.unmap();

    device->createImage(tex_width, tex_height, _mip_levels, VK_SAMPLE_COUNT_1_BIT, _format, VK_IMAGE_TILING_OPTIMAL,
                        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _image, _imageMemory);

    device->singleTimeCommands()
        .transitionImageLayout(_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, _mip_levels)
        .copyBufferToImage(stagingBuffer.buffer(), _image, static_cast<uint32_t>(tex_width), static_cast<uint32_t>(tex_height))
        // TODO
        // Save and load mipmaps from a file
        .generateMipmaps(_image, _format, tex_width, tex_height, _mip_levels)
        .run();

    _imageView = device->createImageView(_image, _format, VK_IMAGE_ASPECT_COLOR_BIT, _mip_levels);

    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = _imageView;
    imageInfo.sampler = nullptr;
}

Image::~Image() {
    vkDestroyImageView(Device::device->vk_device(), _imageView, nullptr);
    vkDestroyImage(Device::device->vk_device(), _image, nullptr);
    vkFreeMemory(Device::device->vk_device(), _imageMemory, nullptr);
}

Sampler::Sampler(GLTF* model, uint32_t samplerID, uint32_t mipLevels) : model{model}, samplerID{samplerID}, mipLevels{mipLevels} {

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    if (model != nullptr) {
        samplerInfo.magFilter = switch_filter(model->samplers[samplerID].magFilter);
        samplerInfo.minFilter = switch_filter(model->samplers[samplerID].minFilter);
        samplerInfo.addressModeU = switch_wrap(model->samplers[samplerID].wrapS);
        samplerInfo.addressModeV = switch_wrap(model->samplers[samplerID].wrapT);
        // gltf doesn't have support for w coordinates
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    } else {
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }
    samplerInfo.anisotropyEnable = VK_TRUE;

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(Device::device->physical_device(), &properties);
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<float>(mipLevels);

    checkResult(vkCreateSampler(Device::device->vk_device(), &samplerInfo, nullptr, &_imageSampler), "failed to create texture sampler!");
}

Sampler::Sampler(uint32_t mipLevels) : mipLevels{mipLevels} {

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(Device::device->physical_device(), &properties);
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<float>(mipLevels);

    checkResult(vkCreateSampler(Device::device->vk_device(), &samplerInfo, nullptr, &_imageSampler), "failed to create texture sampler!");
}

Sampler::~Sampler() { vkDestroySampler(Device::device->vk_device(), _imageSampler, nullptr); }

bool operator==(const Sampler& s1, const Sampler& s2) {
    return s1.model == s2.model && s1.samplerID == s2.samplerID && s1.mipLevels == s2.mipLevels;
}
