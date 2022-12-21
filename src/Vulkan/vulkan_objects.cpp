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
    : device{device}, descriptorManager{descriptorManager}, materialDescriptor(descriptorManager, "material"),
      objectDescriptor(descriptorManager, "object"), computeDescriptor(descriptorManager, "compute") {
    auto startTime = std::chrono::high_resolution_clock::now();
    // Load models
    uint32_t fileNum = 0;
    const std::string baseDir = "assets/glTF/";
    for (const std::filesystem::directory_entry& filePath : std::filesystem::recursive_directory_iterator(baseDir)) {
        // For some reason, !filePath.is_regular_file() isn't short circuiting
        // so it will try to get the file extension of a directory if these are in the same if statement
        if (filePath.exists() && filePath.is_regular_file()) {
            if ((GLTF::getFileExtension(filePath.path()).compare(".gltf") == 0) ||
                (GLTF::getFileExtension(filePath.path()).compare(".glb") == 0)) {
                futureModels.push_back(std::async(
                    std::launch::async, [filePath, fileNum]() { return std::make_shared<VulkanModel>(filePath.path(), fileNum); }));
                ++fileNum;
            }
        }
    }

    for (int modelIndex = 0; modelIndex < futureModels.size(); ++modelIndex) {
        std::shared_ptr<VulkanModel> model = futureModels[modelIndex].get();
        models.insert({model->model->path() + model->model->fileName(), model});
    }

    indirectDraws.resize(GLTF::primitiveCount);

    ssboBuffers = std::make_shared<SSBOBuffers>(device, GLTF::baseInstanceCount, GLTF::primitiveCount);
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
    for (std::pair<std::string, std::shared_ptr<VulkanModel>> pathModelPair : models) {
        std::string filePath = pathModelPair.first;
        std::shared_ptr<VulkanModel> model = pathModelPair.second;

        futureObjects.push_back(std::async(std::launch::async, [model, filePath, this]() {
            return std::make_shared<VulkanObject>(model, ssboBuffers, filePath, indirectDraws);
        }));
    }

    for (int objectIndex = 0; objectIndex < futureObjects.size(); ++objectIndex) {
        objects.push_back(futureObjects[objectIndex].get());
        std::shared_ptr<GLTF> model = objects.back()->model;
        std::string filePath = model->path() + model->fileName();

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
        for (std::pair<int, std::shared_ptr<VulkanMesh>> mesh : models[objects.back()->name()]->meshIDMap) {
            for (std::shared_ptr<VulkanMesh::Primitive> primitive : mesh.second->primitives) {
                indirectDraws[primitive->indirectDrawIndex].vertexOffset = vertices.size();
                indirectDraws[primitive->indirectDrawIndex].firstIndex = indices.size();
                vertices.insert(std::end(vertices), std::begin(primitive->vertices), std::end(primitive->vertices));
                indices.insert(std::end(indices), std::begin(primitive->indices), std::end(primitive->indices));
            }
        }
    }
    vertexBuffer = std::make_shared<StagedBuffer>(device, (void*)vertices.data(), sizeof(vertices[0]) * vertices.size(),
                                                  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    indexBuffer = std::make_shared<StagedBuffer>(device, (void*)indices.data(), sizeof(indices[0]) * indices.size(),
                                                 VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

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

    materialDescriptor.addBinding(0, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, samplerInfos);
    materialDescriptor.addBinding(1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT, imageInfos);
    materialDescriptor.addBinding(2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT, normalMapInfos);
    materialDescriptor.addBinding(3, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT, metallicRoughnessMapInfos);
    materialDescriptor.addBinding(4, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT, aoMapInfos);

    materialDescriptor.allocateSets();
    materialDescriptor.update();

    objectDescriptor.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, ssboBuffers->ssboBuffer());
    objectDescriptor.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, ssboBuffers->materialBuffer());
    culledInstanceIndicesBuffer = std::make_shared<VulkanBuffer>(device, sizeof(InstanceIndicesData) * GLTF::baseInstanceCount,
                                                                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    objectDescriptor.addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, culledInstanceIndicesBuffer->buffer);
    objectDescriptor.addBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, ssboBuffers->materialIndicesBuffer());

    objectDescriptor.allocateSets();
    objectDescriptor.update();

    // NOTE: VK_BUFFER_USAGE_STORAGE_BUFFER_BIT for compute shader to read from it
    indirectDrawsBuffer =
        std::make_shared<StagedBuffer>(device, (void*)indirectDraws.data(), sizeof(indirectDraws[0]) * indirectDraws.size(),
                                       VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

    computeDescriptor.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, indirectDrawsBuffer->getBuffer());
    computeDescriptor.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, ssboBuffers->instanceIndicesBuffer());

    culledIndirectDrawsBuffer = std::make_shared<VulkanBuffer>(device, sizeof(indirectDraws[0]) * indirectDraws.size(),
                                                               VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    computeDescriptor.addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, culledIndirectDrawsBuffer->buffer);
    computeDescriptor.addBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, culledInstanceIndicesBuffer->buffer);

    indirectDrawCountBuffer = std::make_shared<VulkanBuffer>(device, sizeof(uint32_t),
                                                             VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                                                 VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    computeDescriptor.addBinding(4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, indirectDrawCountBuffer->buffer);
    computeDescriptor.addBinding(5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, ssboBuffers->ssboBuffer());

    computeDescriptor.allocateSets();
    computeDescriptor.update();

    auto endTime = std::chrono::high_resolution_clock::now();
    std::cout << "Loaded " << objects.size() << " objects in "
              << std::chrono::duration<float, std::chrono::milliseconds::period>(endTime - startTime).count() << "ms" << std::endl;
}

void VulkanObjects::bind(VulkanRenderer* renderer) {
    VkBuffer vertexBuffers[] = {vertexBuffer->getBuffer()};
    VkDeviceSize offsets[] = {0};

    renderer->bindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->graphicsPipelineLayout(), 1, materialDescriptor.getSets()[0]);
    renderer->bindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->graphicsPipelineLayout(), 2, objectDescriptor.getSets()[0]);

    vkCmdBindVertexBuffers(renderer->getCurrentCommandBuffer(), 0, 1, vertexBuffers, offsets);

    vkCmdBindIndexBuffer(renderer->getCurrentCommandBuffer(), indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
}

void VulkanObjects::drawIndirect(VulkanRenderer* renderer) {
    for (std::shared_ptr<VulkanObject> animatedObject : animatedObjects) {
        animatedObject->updateAnimations();
    }

    vkCmdDrawIndexedIndirectCount(renderer->getCurrentCommandBuffer(), culledIndirectDrawsBuffer->buffer, 0,
                                  indirectDrawCountBuffer->buffer, 0, indirectDraws.size(), sizeof(indirectDraws[0]));
}

std::shared_ptr<VulkanObject> VulkanObjects::getObjectByName(std::string name) {
    for (std::shared_ptr<VulkanObject> object : objects) {
        if (object->name().compare(name) == 0) {
            return object;
        }
    }
    throw std::runtime_error("failed to find object by name: " + name);
}
