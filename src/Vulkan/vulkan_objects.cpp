#include "vulkan_objects.hpp"
#include "../glTF/base64.hpp"
#include "common.hpp"
#include "vulkan_buffer.hpp"
#include "vulkan_descriptors.hpp"
#include "vulkan_device.hpp"
#include "vulkan_image.hpp"
#include "vulkan_object.hpp"
#include "vulkan_renderer.hpp"
#include <chrono>
#include <filesystem>
#include <glm/gtx/string_cast.hpp>
#include <iterator>
#include <memory>
#include <vulkan/vulkan_core.h>

VulkanObjects::VulkanObjects(VulkanDevice* device, VulkanDescriptors* descriptorManager)
    : device{device}, descriptorManager{descriptorManager} {
    // Load models
    uint32_t fileNum = 0;
    const std::string baseDir = "assets/glTF/";
    for (const std::filesystem::directory_entry& filePath : std::filesystem::recursive_directory_iterator(baseDir)) {
        // For some reason, !filePath.is_regular_file() isn't short circuiting
        // so it will try to get the file extension of a directory if these are in the same if statement
        if (filePath.exists() && filePath.is_regular_file()) {
            if ((GLTF::getFileExtension(filePath.path()).compare(".gltf") == 0) ||
                (GLTF::getFileExtension(filePath.path()).compare(".glb") == 0)) {
                std::shared_ptr<GLTF> model = std::make_shared<GLTF>(filePath.path(), fileNum);
                ++fileNum;
                gltf_models.insert({filePath.path(), model});
            }
        }
    }

    // TODO
    // object and material buffers don't need to be the same size
    ssboBuffers = std::make_shared<SSBOBuffers>(device, GLTF::baseInstanceCount, GLTF::materialCount);
    ssboBuffers->defaultImage = std::make_shared<VulkanImage>(device, "assets/pixels/white_pixel.png");
    ssboBuffers->defaultSampler =
        std::make_shared<VulkanSampler>(device, reinterpret_cast<VulkanImage*>(ssboBuffers->defaultImage.get())->mipLevels());
    ssboBuffers->defaultNormalMap = std::make_shared<VulkanImage>(device, "assets/pixels/blue_pixel.png", VK_FORMAT_R8G8B8A8_UNORM);
    ssboBuffers->defaultMetallicRoughnessMap =
        std::make_shared<VulkanImage>(device, "assets/pixels/green_pixel.png", VK_FORMAT_R8G8B8A8_UNORM);
    ssboBuffers->defaultAoMap = std::make_shared<VulkanImage>(device, "assets/pixels/white_pixel.png", VK_FORMAT_R8G8B8A8_UNORM);
    ssboBuffers->uniqueImagesMap.insert({(void*)ssboBuffers->defaultImage.get(), 0});
    ssboBuffers->uniqueSamplersMap.insert({(void*)ssboBuffers->defaultSampler.get(), 0});
    ssboBuffers->uniqueNormalMapsMap.insert({(void*)ssboBuffers->defaultNormalMap.get(), 0});
    ssboBuffers->uniqueMetallicRoughnessMapsMap.insert({(void*)ssboBuffers->defaultMetallicRoughnessMap.get(), 0});
    ssboBuffers->uniqueAoMapsMap.insert({(void*)ssboBuffers->defaultAoMap.get(), 0});

    // Load objects
    // This needs to be in a separate loop from loading models in order to dynamically size ssboBuffers
    for (std::pair<std::string, std::shared_ptr<GLTF>> pathModelPair : gltf_models) {
        std::string filePath = pathModelPair.first;
        std::shared_ptr<GLTF> model = pathModelPair.second;

        objects.push_back(std::make_shared<VulkanObject>(model, ssboBuffers, filePath, indirectDraws));
        if (model->animations.size() > 0) {
            animatedObjects.push_back(objects.back());
        }
        if (filePath == (baseDir + "TriangleWithoutIndices.gltf")) {
            objects.back()->x(-3.0f);
        }
        if (filePath == (baseDir + "simple_meshes.gltf")) {
            objects.back()->x(5.0f);
        }
        if (filePath == (baseDir + "basic_sparse_triangles.gltf")) {
            objects.back()->y(2.0f);
        }
        if (filePath == (baseDir + "simple_animation.gltf")) {
            objects.back()->x(-3.0f);
            objects.back()->y(3.0f);
        }
        if (filePath == (baseDir + "Box.glb")) {
            objects.back()->y(-3.0f);
        }
        if (filePath == (baseDir + "GearboxAssy.glb")) {
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
        if (filePath == (baseDir + "2CylinderEngine.glb")) {
            objects.back()->setScale({0.01f, 0.01f, 0.01f});
            objects.back()->y(5.0f);
        }
        if (filePath == (baseDir + "simple_material.gltf")) {
            objects.back()->x(3.0f);
        }
        if (filePath == (baseDir + "simple_texture.gltf")) {
            objects.back()->x(3.0f);
            objects.back()->y(-3.0f);
        }
        if (filePath == (baseDir + "ABeautifulGame/ABeautifulGame.gltf")) {
            objects.back()->z(5.0f);
            objects.back()->setScale({5.0f, 5.0f, 5.0f});
        }
        if (filePath == (baseDir + "uss_enterprise_d_star_trek_tng.glb")) {
            objects.back()->z(5.0f);
            objects.back()->y(-5.0f);
        }
        if (filePath == (baseDir + "WaterBottle.glb")) {
            objects.back()->z(-3.0f);
        }
        for (std::pair<int, std::shared_ptr<VulkanMesh>> mesh : objects.back()->meshIDMap) {
            for (std::shared_ptr<VulkanMesh::Primitive> primitive : mesh.second->primitives) {
                indirectDraws[primitive->indirectDrawIndex].vertexOffset = vertices.size();
                indirectDraws[primitive->indirectDrawIndex].firstIndex = indices.size();
                vertices.insert(std::end(vertices), std::begin(primitive->vertices), std::end(primitive->vertices));
                indices.insert(std::end(indices), std::begin(primitive->indices), std::end(primitive->indices));
            }
        }
    }
    // TODO
    // Optimize final vertex and index buffers
    vertexBuffer = std::make_shared<StagedBuffer>(device, (void*)vertices.data(), sizeof(vertices[0]) * vertices.size(),
                                                  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    indexBuffer = std::make_shared<StagedBuffer>(device, (void*)indices.data(), sizeof(indices[0]) * indices.size(),
                                                 VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    std::vector<VkWriteDescriptorSet> descriptorWrites(5);

    // Get unique samplers and load into continuous vector
    samplerInfos.resize(ssboBuffers->uniqueSamplersMap.size());
    for (auto it = ssboBuffers->uniqueSamplersMap.begin(); it != ssboBuffers->uniqueSamplersMap.end(); ++it) {
        samplerInfos[it->second].sampler = reinterpret_cast<VulkanSampler*>(it->first)->imageSampler();
    }
    imageInfos.resize(ssboBuffers->uniqueImagesMap.size());
    for (auto it = ssboBuffers->uniqueImagesMap.begin(); it != ssboBuffers->uniqueImagesMap.end(); ++it) {
        imageInfos[it->second] = reinterpret_cast<VulkanImage*>(it->first)->imageInfo;
    }
    normalMapInfos.resize(ssboBuffers->uniqueNormalMapsMap.size());
    for (auto it = ssboBuffers->uniqueNormalMapsMap.begin(); it != ssboBuffers->uniqueNormalMapsMap.end(); ++it) {
        normalMapInfos[it->second] = reinterpret_cast<VulkanImage*>(it->first)->imageInfo;
    }
    metallicRoughnessMapInfos.resize(ssboBuffers->uniqueMetallicRoughnessMapsMap.size());
    for (auto it = ssboBuffers->uniqueMetallicRoughnessMapsMap.begin(); it != ssboBuffers->uniqueMetallicRoughnessMapsMap.end(); ++it) {
        metallicRoughnessMapInfos[it->second] = reinterpret_cast<VulkanImage*>(it->first)->imageInfo;
    }
    aoMapInfos.resize(ssboBuffers->uniqueAoMapsMap.size());
    for (auto it = ssboBuffers->uniqueAoMapsMap.begin(); it != ssboBuffers->uniqueAoMapsMap.end(); ++it) {
        aoMapInfos[it->second] = reinterpret_cast<VulkanImage*>(it->first)->imageInfo;
    }
    // Material layout
    std::vector<VkDescriptorSetLayoutBinding> materialBindings = descriptorManager->materialLayout(
        samplerInfos.size(), imageInfos.size(), normalMapInfos.size(), metallicRoughnessMapInfos.size(), aoMapInfos.size());
    materialSet = descriptorManager->allocateSet(descriptorManager->createLayout(materialBindings, 1));

    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = materialSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    descriptorWrites[0].descriptorCount = samplerInfos.size();
    descriptorWrites[0].pImageInfo = samplerInfos.data();

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = materialSet;
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    descriptorWrites[1].descriptorCount = imageInfos.size();
    descriptorWrites[1].pImageInfo = imageInfos.data();

    descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[2].dstSet = materialSet;
    descriptorWrites[2].dstBinding = 2;
    descriptorWrites[2].dstArrayElement = 0;
    descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    descriptorWrites[2].descriptorCount = normalMapInfos.size();
    descriptorWrites[2].pImageInfo = normalMapInfos.data();

    descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[3].dstSet = materialSet;
    descriptorWrites[3].dstBinding = 3;
    descriptorWrites[3].dstArrayElement = 0;
    descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    descriptorWrites[3].descriptorCount = metallicRoughnessMapInfos.size();
    descriptorWrites[3].pImageInfo = metallicRoughnessMapInfos.data();

    descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[4].dstSet = materialSet;
    descriptorWrites[4].dstBinding = 4;
    descriptorWrites[4].dstArrayElement = 0;
    descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    descriptorWrites[4].descriptorCount = aoMapInfos.size();
    descriptorWrites[4].pImageInfo = aoMapInfos.data();

    vkUpdateDescriptorSets(device->device(), 5, descriptorWrites.data(), 0, nullptr);

    objectSet = descriptorManager->allocateSet(descriptorManager->getObject());
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

    VkDescriptorBufferInfo instanceIndicesBufferInfo{};
    instanceIndicesBufferInfo.buffer = ssboBuffers->instanceIndicesBuffer();
    instanceIndicesBufferInfo.offset = 0;
    instanceIndicesBufferInfo.range = VK_WHOLE_SIZE;

    descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[2].dstSet = objectSet;
    descriptorWrites[2].dstBinding = 3;
    descriptorWrites[2].dstArrayElement = 0;
    descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrites[2].descriptorCount = 1;
    descriptorWrites[2].pBufferInfo = &instanceIndicesBufferInfo;

    VkDescriptorBufferInfo materialIndicesBufferInfo{};
    materialIndicesBufferInfo.buffer = ssboBuffers->materialIndicesBuffer();
    materialIndicesBufferInfo.offset = 0;
    materialIndicesBufferInfo.range = VK_WHOLE_SIZE;

    descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[3].dstSet = objectSet;
    descriptorWrites[3].dstBinding = 4;
    descriptorWrites[3].dstArrayElement = 0;
    descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrites[3].descriptorCount = 1;
    descriptorWrites[3].pBufferInfo = &materialIndicesBufferInfo;

    vkUpdateDescriptorSets(device->device(), 4, descriptorWrites.data(), 0, nullptr);

    indirectDrawsBuffer = std::make_shared<StagedBuffer>(
        device, (void*)indirectDraws.data(), sizeof(indirectDraws[0]) * indirectDraws.size(), VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);
    std::cout << "objects loaded: " << objects.size() << std::endl;
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

std::shared_ptr<VulkanObject> VulkanObjects::getObjectByName(std::string name) {
    for (std::shared_ptr<VulkanObject> object : objects) {
        if (object->name().compare(name) == 0) {
            return object;
        }
    }
    throw std::runtime_error("failed to find object by name: " + name);
}
