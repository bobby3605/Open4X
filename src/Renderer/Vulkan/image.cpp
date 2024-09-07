#include "image.hpp"
#include "command_runner.hpp"
#include "common.hpp"
#include "fastgltf/types.hpp"
#include "memory_manager.hpp"
#include <fcntl.h>
#include <filesystem>
#include <stdexcept>
#include <variant>
#include <vulkan/vulkan_core.h>
#define STB_IMAGE_IMPLEMENTATION
#include "../Allocator/base_allocator.hpp"
#include "stb/stb_image.h"

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

VkFilter switch_filter(uint16_t filter) {
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

VkSamplerMipmapMode switch_mipmap_filter(uint16_t filter) {
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
                     sizeof(_tex_width) + sizeof(_tex_height) + sizeof(_tex_channels) + _tex_width * _tex_height * 4);
    if (!mmio.is_open()) {
        std::cout << "WARNING: "
                  << "failed to write image cache: " << directory + file_name << std::endl;
    }
    mmio.write(_tex_width);
    mmio.write(_tex_height);
    mmio.write(_tex_channels);
    mmio.write(_pixels, _tex_width * _tex_height * 4);
}

bool Image::read_image(std::string path) {
    MMIO mmio = MMIO("assets/cache/images/" + path + std::string(".imageCache"), O_RDONLY);
    if (!mmio.is_open()) {
        return false;
    }
    mmio.read(_tex_width);
    mmio.read(_tex_height);
    mmio.read(_tex_channels);
    // NOTE:
    // _pixel_read_buffer should never be changed after this point,
    // if it does, then pixels should get a new pointer,
    // since vector operations can change the base memory location
    _pixel_read_buffer.reserve(_tex_width * _tex_height * 4);
    _pixels = _pixel_read_buffer.data();
    mmio.read(_pixels, _tex_width * _tex_height * 4);
    return true;
}

Image::Image(fastgltf::Asset const& asset, uint32_t image_ID, std::string relative_gltf_path) {
    // Get the directory path for the image cache
    // Example:
    // relative_path: assets/glTF/ABeautifulGame/ABeautifulGame.gltf
    // file_name: ABeautifulGame.gltf
    // base_path: assets/glTF/ABeautifulGame/
    // gltf_path: glTF/ABeautifulGame/
    // path: ABeautifulGame/
    // cache_base_path: ABeautifulGame/ABeautifulGame-gltf/

    std::string base_path = relative_gltf_path.substr(0, relative_gltf_path.find_last_of("/"));
    std::string file_name = get_filename(relative_gltf_path);
    std::string gltf_path = base_path.substr(base_path.find_first_of("/") + 1);
    std::string path = gltf_path.substr(gltf_path.find_first_of("/") + 1);
    std::string cache_path =
        path + file_name.substr(0, file_name.find_last_of(".")) + "-" + file_name.substr(file_name.find_last_of(".") + 1) + "/";

    if (std::holds_alternative<fastgltf::sources::URI>(asset.images[image_ID].data)) {
        fastgltf::URI const& uri = std::get<fastgltf::sources::URI>(asset.images[image_ID].data).uri;
        // add the uri to the cache path
        cache_path += uri.string();
        // check if the image file has been loaded from the cache
        if (!read_image(cache_path)) {
            // if it hasn't been loaded,
            // load it from the gltf image and cache it
            _pixels = stbi_load((base_path + uri.c_str()).c_str(), &_tex_width, &_tex_height, &_tex_channels, STBI_rgb_alpha);
            if (!_pixels) {
                throw std::runtime_error("failed to load texture image: " + base_path + uri.c_str());
            }
            _stbi_flag = true;
            write_image(cache_path);
        }
    } else if (std::holds_alternative<fastgltf::sources::BufferView>(asset.images[image_ID].data)) {
        throw std::runtime_error("TODO buffer view images");

        fastgltf::sources::BufferView const& buffer_view_sources = std::get<fastgltf::sources::BufferView>(asset.images[image_ID].data);
        // add bufferView-n to the directory path
        cache_path += "bufferView-" + std::to_string(buffer_view_sources.bufferViewIndex);
        if (!read_image(cache_path)) {
            fastgltf::BufferView const& buffer_view = asset.bufferViews[buffer_view_sources.bufferViewIndex];
            if (buffer_view.byteStride.has_value()) {
                throw std::runtime_error("byte stride unsupported on image buffer views: " + cache_path);
            }
            asset.buffers[buffer_view.bufferIndex].data;
            /*
            _pixels = stbi_load_from_memory(model->buffers[bufferView->buffer].data.data() + bufferView->byteOffset, bufferView->byteLength,
                                            &_tex_width, &_tex_height, &_tex_channels, STBI_rgb_alpha);
            if (!_pixels) {
                throw std::runtime_error("failed to load texture image from bufferView: " +
                                         std::to_string(model->images[sourceID].bufferView.value()));
            }
                                            */
            _stbi_flag = true;
            write_image(cache_path);
        }
    } else if (std::holds_alternative<fastgltf::sources::Array>(asset.images[image_ID].data)) {
        fastgltf::sources::Array const& array_source = std::get<fastgltf::sources::Array>(asset.images[image_ID].data);
        _pixels = stbi_load_from_memory(array_source.bytes.data(), array_source.bytes.size_bytes(), &_tex_width, &_tex_height,
                                        &_tex_channels, STBI_rgb_alpha);
        if (!_pixels) {
            throw std::runtime_error("failed to load texture image from array: " + base_path + " image id: " + std::to_string(image_ID));
        }
        _stbi_flag = true;
    } else {
        throw std::runtime_error("no data found for image on imageID: " + std::to_string(image_ID) + " file: " + path +
                                 " holding alternative: " + std::to_string(asset.images[image_ID].data.index()));
    }
    // _mip_levels = static_cast<uint32_t>(std::floor(std::log2(std::max(_tex_width, _tex_height)))) + 1;
    _mip_levels = 1;

    load_pixels();
}

Image::Image(std::string path) {
    _pixels = stbi_load(path.c_str(), &_tex_width, &_tex_height, &_tex_channels, STBI_rgb_alpha);
    if (!_pixels) {
        throw std::runtime_error("failed to load texture image");
    }
    //_mip_levels = static_cast<uint32_t>(std::floor(std::log2(std::max(_tex_width, _tex_height)))) + 1;
    _mip_levels = 1;
    load_pixels();
}
// TODO:
// set debug name for image, image view, and sampler

void Image::load_pixels() {
    VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
    VkSampleCountFlagBits num_samples = VK_SAMPLE_COUNT_1_BIT;
    VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
    VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    VkImageAspectFlags aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT;

    VkImageCreateInfo image_create_info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.extent.width = width();
    image_create_info.extent.height = height();
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = mip_levels();
    image_create_info.arrayLayers = 1;
    image_create_info.format = format;
    image_create_info.tiling = tiling;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_create_info.usage = usage;
    image_create_info.samples = num_samples;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.requiredFlags = properties;
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
    check_result(vmaCreateImage(Device::device->vma_allocator(), &image_create_info, &alloc_info, &_vk_image, &_vma_allocation, nullptr),
                 "failed to create image!");

    VkImageViewCreateInfo view_info{};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = _vk_image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = format;
    view_info.subresourceRange.aspectMask = aspect_flags;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = mip_levels();
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;

    check_result(vkCreateImageView(Device::device->vk_device(), &view_info, nullptr, &_vk_image_view),
                 "failed to create texture image view!");

    GPUAllocation staging_buffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "");

    staging_buffer.realloc(width() * height() * 4);
    staging_buffer.write(0, pixels(), width() * height() * 4);

    CommandRunner process_image;
    process_image.transition_image(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mip_levels(), _vk_image);
    process_image.copy_buffer_to_image(staging_buffer.buffer(), _vk_image, width(), height());
    process_image.transition_image(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mip_levels(), _vk_image);
    process_image.run();

    /*
    void* mapped;
    vmaMapMemory(Device::device->vma_allocator(), _vma_allocation, &mapped);
    std::memcpy(reinterpret_cast<unsigned char*>(mapped), pixels(), width() * height() * 4);
    vmaUnmapMemory(Device::device->vma_allocator(), _vma_allocation);
    */

    _image_info.imageView = _vk_image_view;
    /*
    // TODO
    // Save and load mipmaps from a file
    //        .generateMipmaps(_image, _format, tex_width, _tex_height, _mip_levels)
    */
    _image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

Image::~Image() {
    if (_stbi_flag) {
        stbi_image_free(_pixels);
    }
    vmaDestroyImage(Device::device->vma_allocator(), _vk_image, _vma_allocation);
    vkDestroyImageView(Device::device->vk_device(), _vk_image_view, nullptr);
}

Sampler::Sampler(fastgltf::Asset const& asset, uint32_t sampler_ID, uint32_t mip_levels) {

    VkSamplerCreateInfo sampler_info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    if (asset.samplers[sampler_ID].magFilter.has_value()) {
        sampler_info.magFilter = switch_filter(static_cast<uint16_t>(asset.samplers[sampler_ID].magFilter.value()));
    } else {
        sampler_info.magFilter = VK_FILTER_LINEAR;
    }
    if (asset.samplers[sampler_ID].minFilter.has_value()) {
        sampler_info.minFilter = switch_filter(static_cast<uint16_t>(asset.samplers[sampler_ID].minFilter.value()));
    } else {
        sampler_info.minFilter = VK_FILTER_LINEAR;
    }
    sampler_info.addressModeU = switch_wrap(static_cast<uint16_t>(asset.samplers[sampler_ID].wrapS));
    sampler_info.addressModeV = switch_wrap(static_cast<uint16_t>(asset.samplers[sampler_ID].wrapT));
    // gltf doesn't have support for w coordinates
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.anisotropyEnable = VK_TRUE;

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(Device::device->physical_device(), &properties);
    sampler_info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = static_cast<float>(mip_levels);

    check_result(vkCreateSampler(Device::device->vk_device(), &sampler_info, nullptr, &_vk_sampler), "failed to create texture sampler!");

    _image_info.sampler = _vk_sampler;
}

Sampler::Sampler(uint32_t mip_levels) {

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
    samplerInfo.maxLod = static_cast<float>(mip_levels);

    check_result(vkCreateSampler(Device::device->vk_device(), &samplerInfo, nullptr, &_vk_sampler), "failed to create texture sampler!");

    _image_info.sampler = _vk_sampler;
}

Sampler::~Sampler() { vkDestroySampler(Device::device->vk_device(), _vk_sampler, nullptr); }
