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

VulkanObjects::~VulkanObjects() {

    vkDestroySampler(device->device(), imageSampler, nullptr);
    vkDestroyImageView(device->device(), imageView, nullptr);
    vkDestroyImage(device->device(), image, nullptr);
    vkFreeMemory(device->device(), imageMemory, nullptr);
}

VulkanObjects::VulkanObjects(VulkanDevice* device, VulkanDescriptors* descriptorManager)
    : device{device}, descriptorManager{descriptorManager} {
    // TODO
    // ssboBuffers should be dynamically sized
    // also, the object and material buffers don't need to be the same size
    ssboBuffers = std::make_shared<SSBOBuffers>(device, 1000);
    uint32_t fileNum = 0;
    for (const auto& filePath : std::filesystem::directory_iterator("assets/glTF/")) {
        if (filePath.exists() && filePath.is_regular_file() && (GLTF::getFileExtension(filePath.path()).compare(".gltf") == 0) ||
            GLTF::getFileExtension(filePath.path()).compare(".glb") == 0) {
            std::shared_ptr<GLTF> model = std::make_shared<GLTF>(filePath.path(), fileNum);
            ++fileNum;
            gltf_models.insert({filePath.path(), model});
            objects.push_back(std::make_shared<VulkanObject>(model, ssboBuffers));
            if (model->animations.size() > 0) {
                animatedObjects.push_back(objects.back());
            }
            if (filePath.path() == "assets/glTF/TriangleWithoutIndices.gltf") {
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
            if (filePath.path() == "assets/glTF/Box.glb") {
                objects.back()->y(-3.0f);
            }
            if (filePath.path() == "assets/glTF/GearboxAssy.glb") {
                // FIXME:
                // setting the scale and position seem to be affected by the dimensions of the model
                // maybe the model has a large offset? the position for the first node in the gltf version of it is 155,8,-37
                // setting the scale to 0.1 and translation of 0,0,0 would give a translation of 15.5, 0.8, 3.7,
                // which is close to the real position that it shows up at
                objects.back()->x(0.0f);
                objects.back()->y(0.0f);
                objects.back()->z(0.0f);
                objects.back()->setScale({0.1f, 0.1f, 0.1f});
            }
            if (filePath.path() == "assets/glTF/2CylinderEngine.glb") {
                objects.back()->setScale({0.01f, 0.01f, 0.01f});
                objects.back()->y(5.0f);
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

    std::vector<VkWriteDescriptorSet> descriptorWrites(3);

    VkDescriptorBufferInfo ssboInfo{};
    ssboInfo.buffer = ssboBuffers->ssboBuffer();
    ssboInfo.offset = 0;
    ssboInfo.range = VK_WHOLE_SIZE;

    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = objectSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &ssboInfo;

    VkDescriptorBufferInfo materialBufferInfo{};
    materialBufferInfo.buffer = ssboBuffers->materialBuffer();
    materialBufferInfo.offset = 0;
    materialBufferInfo.range = VK_WHOLE_SIZE;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = objectSet;
    descriptorWrites[1].dstBinding = 2;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pBufferInfo = &materialBufferInfo;

    VkDescriptorBufferInfo indicesBufferInfo{};
    indicesBufferInfo.buffer = ssboBuffers->indicesBuffer();
    indicesBufferInfo.offset = 0;
    indicesBufferInfo.range = VK_WHOLE_SIZE;

    descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[2].dstSet = objectSet;
    descriptorWrites[2].dstBinding = 3;
    descriptorWrites[2].dstArrayElement = 0;
    descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrites[2].descriptorCount = 1;
    descriptorWrites[2].pBufferInfo = &indicesBufferInfo;

    vkUpdateDescriptorSets(device->device(), 3, descriptorWrites.data(), 0, nullptr);

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
