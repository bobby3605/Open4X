#include "vulkan_objects.hpp"
#include "../../external/stb_image.h"
#include "../glTF/base64.hpp"
#include "common.hpp"
#include "vulkan_buffer.hpp"
#include "vulkan_descriptors.hpp"
#include "vulkan_device.hpp"
#include "vulkan_object.hpp"
#include "vulkan_renderer.hpp"
#include <filesystem>
#include <glm/gtx/string_cast.hpp>
#include <iterator>
#include <memory>
#include <vulkan/vulkan_core.h>

std::string getFileExtension(std::string filePath) { return filePath.substr(filePath.find_last_of(".")); }

VulkanObjects::~VulkanObjects() {

    vkDestroySampler(device->device(), imageSampler, nullptr);
    vkDestroyImageView(device->device(), imageView, nullptr);
    vkDestroyImage(device->device(), image, nullptr);
    vkFreeMemory(device->device(), imageMemory, nullptr);
}

VulkanObjects::VulkanObjects(VulkanDevice* device, VulkanDescriptors* descriptorManager)
    : device{device}, descriptorManager{descriptorManager} {
    SSBO = std::make_shared<SSBOBuffers>(device, sizeof(SSBOData) * 1000);
    for (const auto& filePath : std::filesystem::directory_iterator("assets/glTF/")) {
        if (filePath.exists() && filePath.is_regular_file() && getFileExtension(filePath.path()).compare(".gltf") == 0) {
            std::shared_ptr<GLTF> model = std::make_shared<GLTF>(filePath.path());
            gltf_models.insert({filePath.path(), model});
            objects.push_back(std::make_shared<VulkanObject>(model, SSBO));
            if (model->animations.size() > 0) {
                animatedObjects.push_back(objects.back());
            }
            if (filePath.path() == "assets/glTF/basic_triangle.gltf") {
                objects.back()->x(-3.0f);
            }
            if (filePath.path() == "assets/glTF/simple_meshes.gltf") {
                objects.back()->x(5.0f);
            }
            if (filePath.path() == "assets/glTF/basic_sparse_triangles.gltf") {
                objects.back()->y(2.0f);
            }
            if (filePath.path() == "assets/glTF/simple_animation.gltf") {
                objects.back()->x(-3.0f);
                objects.back()->y(3.0f);
            }
            for (std::pair<int, std::shared_ptr<VulkanMesh>> mesh : *objects.back()->rootNodes[0]->meshIDMap) {
                for (std::shared_ptr<VulkanMesh::Primitive> primitive : mesh.second->primitives) {
                    primitive->indirectDraw.vertexOffset = vertices.size();
                    primitive->indirectDraw.firstIndex = indices.size();
                    indirectDraws.push_back(primitive->indirectDraw);
                    vertices.insert(std::end(vertices), std::begin(primitive->vertices), std::end(primitive->vertices));
                    indices.insert(std::end(indices), std::begin(primitive->indices), std::end(primitive->indices));
                }
            }
        }
    }
    // TODO
    // Optimize final vertex and index buffers
    vertexBuffer = std::make_shared<StagedBuffer>(device, (void*)vertices.data(), sizeof(vertices[0]) * vertices.size(),
                                                  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    indexBuffer = std::make_shared<StagedBuffer>(device, (void*)indices.data(), sizeof(indices[0]) * indices.size(),
                                                 VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    objectSet = descriptorManager->allocateSet(descriptorManager->getObject());

    std::vector<VkWriteDescriptorSet> descriptorWrites(1);

    VkDescriptorBufferInfo ssboInfo{};
    ssboInfo.buffer = SSBO->buffer();
    ssboInfo.offset = 0;
    ssboInfo.range = VK_WHOLE_SIZE;

    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = objectSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &ssboInfo;

    vkUpdateDescriptorSets(device->device(), 1, descriptorWrites.data(), 0, nullptr);

    indirectDrawsBuffer = std::make_shared<StagedBuffer>(
        device, (void*)indirectDraws.data(), sizeof(indirectDraws[0]) * indirectDraws.size(), VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);

    loadImage("assets/textures/white.png");
}

void VulkanObjects::bind(VulkanRenderer* renderer) {
    VkBuffer vertexBuffers[] = {vertexBuffer->getBuffer()};
    VkDeviceSize offsets[] = {0};

    renderer->bindDescriptorSet(1, materialSet);
    renderer->bindDescriptorSet(2, objectSet);

    vkCmdBindVertexBuffers(renderer->getCurrentCommandBuffer(), 0, 1, vertexBuffers, offsets);

    vkCmdBindIndexBuffer(renderer->getCurrentCommandBuffer(), indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
}

void VulkanObjects::drawIndirect(VulkanRenderer* renderer) {
    for (std::shared_ptr<VulkanObject> animatedObject : animatedObjects) {
        animatedObject->updateAnimations();
    }

    vkCmdDrawIndexedIndirect(renderer->getCurrentCommandBuffer(), indirectDrawsBuffer->getBuffer(), 0, indirectDraws.size(),
                             sizeof(indirectDraws[0]));
}

void VulkanObjects::loadImage(std::string path) {

    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
        throw std::runtime_error("failed to load texture image");
    }

    mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

    VulkanBuffer stagingBuffer(device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    stagingBuffer.map();
    stagingBuffer.write(pixels, imageSize);
    stagingBuffer.unmap();

    stbi_image_free(pixels);

    device->createImage(texWidth, texHeight, mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, imageMemory);

    device->singleTimeCommands()
        .transitionImageLayout(image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels)
        .copyBufferToImage(stagingBuffer.buffer, image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight))
        // TODO
        // Save and load mipmaps from a file
        .generateMipmaps(image, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, mipLevels)
        .run();

    imageView = device->createImageView(image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);

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

    checkResult(vkCreateSampler(device->device(), &samplerInfo, nullptr, &imageSampler), "failed to create texture sampler!");

    materialSet = descriptorManager->allocateSet(descriptorManager->getMaterial());

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = imageView;
    imageInfo.sampler = imageSampler;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = materialSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(device->device(), 1, &descriptorWrite, 0, nullptr);
}
