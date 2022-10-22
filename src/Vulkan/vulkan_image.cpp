#include "vulkan_image.hpp"
#include "common.hpp"
#include "vulkan_buffer.hpp"
#include "vulkan_swapchain.hpp"
#include <cmath>
#include <cstdint>

VkSamplerAddressMode switchWrap(uint32_t wrap) {
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

VkFilter switchFilter(uint32_t filter) {
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

VkSamplerMipmapMode switchMipmapFilter(uint32_t filter) {
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

VulkanImage::VulkanImage(VulkanDevice* device, GLTF* model, uint32_t textureID, VkFormat format)
    : device{device}, model{model}, _textureID{textureID}, _format{format} {
    uint32_t sourceID = model->textures[textureID].source;

    if (model->images[sourceID].uri.has_value()) {
        pixels =
            stbi_load((model->path() + model->images[sourceID].uri.value()).c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        if (!pixels) {
            throw std::runtime_error("failed to load texture image: " + model->path() + model->images[sourceID].uri.value());
        }
    } else if (model->images[sourceID].bufferView.has_value()) {
        // FIXME:
        // support loading from a bufferView
        throw std::runtime_error("not supporting image bufferView loading yet");
    } else {
        throw std::runtime_error("no data found for image on textureID: " + std::to_string(textureID));
    }

    loadPixels();
}

VulkanImage::VulkanImage(VulkanDevice* device, std::string path, VkFormat format) : device{device}, _format{format} {
    pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels) {
        throw std::runtime_error("failed to load texture image");
    }
    loadPixels();
}

void VulkanImage::loadPixels() {

    VkDeviceSize imageSize = texWidth * texHeight * 4;
    _mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

    VulkanBuffer stagingBuffer(device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    stagingBuffer.map();
    stagingBuffer.write(pixels, imageSize);
    stagingBuffer.unmap();

    stbi_image_free(pixels);

    device->createImage(texWidth, texHeight, _mipLevels, VK_SAMPLE_COUNT_1_BIT, _format, VK_IMAGE_TILING_OPTIMAL,
                        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _image, _imageMemory);

    device->singleTimeCommands()
        .transitionImageLayout(_image, _format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, _mipLevels)
        .copyBufferToImage(stagingBuffer.buffer, _image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight))
        // TODO
        // Save and load mipmaps from a file
        .generateMipmaps(_image, _format, texWidth, texHeight, _mipLevels)
        .run();

    _imageView = device->createImageView(_image, _format, VK_IMAGE_ASPECT_COLOR_BIT, _mipLevels);

    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = _imageView;
    imageInfo.sampler = nullptr;
}

VulkanImage::~VulkanImage() {
    vkDestroyImageView(device->device(), _imageView, nullptr);
    vkDestroyImage(device->device(), _image, nullptr);
    vkFreeMemory(device->device(), _imageMemory, nullptr);
}

VulkanSampler::VulkanSampler(VulkanDevice* device, GLTF* model, uint32_t samplerID, uint32_t mipLevels)
    : device{device}, model{model}, samplerID{samplerID}, mipLevels{mipLevels} {

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    if (model != nullptr) {
        samplerInfo.magFilter = switchFilter(model->samplers[samplerID].magFilter);
        samplerInfo.minFilter = switchFilter(model->samplers[samplerID].minFilter);
        samplerInfo.addressModeU = switchWrap(model->samplers[samplerID].wrapS);
        samplerInfo.addressModeV = switchWrap(model->samplers[samplerID].wrapT);
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
    vkGetPhysicalDeviceProperties(device->getPhysicalDevice(), &properties);
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<float>(mipLevels);

    checkResult(vkCreateSampler(device->device(), &samplerInfo, nullptr, &_imageSampler), "failed to create texture sampler!");
}

VulkanSampler::VulkanSampler(VulkanDevice* device, uint32_t mipLevels) : device{device}, mipLevels{mipLevels} {

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(device->getPhysicalDevice(), &properties);
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<float>(mipLevels);

    checkResult(vkCreateSampler(device->device(), &samplerInfo, nullptr, &_imageSampler), "failed to create texture sampler!");
}

VulkanSampler::~VulkanSampler() { vkDestroySampler(device->device(), _imageSampler, nullptr); }

bool operator==(const VulkanSampler& s1, const VulkanSampler& s2) {
    return s1.model == s2.model && s1.samplerID == s2.samplerID && s1.mipLevels == s2.mipLevels;
}
