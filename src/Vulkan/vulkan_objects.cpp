#include "vulkan_objects.hpp"
#include "../glTF/base64.hpp"
#include "common.hpp"
#include "vulkan_buffer.hpp"
#include "vulkan_descriptors.hpp"
#include "vulkan_device.hpp"
#include "vulkan_image.hpp"
#include "vulkan_node.hpp"
#include "vulkan_object.hpp"
#include "vulkan_renderer.hpp"
#include <chrono>
#include <filesystem>
#include <glm/gtx/string_cast.hpp>
#include <iterator>
#include <memory>
#include <random>
#include <vulkan/vulkan_core.h>
#include <execution>
#include <algorithm>

VulkanObjects::VulkanObjects(VulkanDevice* device, VulkanDescriptors* descriptorManager, Settings settings)
    : device{device}, descriptorManager{descriptorManager} {
    _totalInstanceCount = 0;
    auto startTime = std::chrono::high_resolution_clock::now();
    // Load models
    uint32_t fileNum = 0;
    const std::string baseDir = "assets/glTF/";

    // FIXME:
//    ssboBuffers = std::make_shared<SSBOBuffers>(device, GLTF::primitiveCount);
//    remove model dependence on ssboBuffers, or split ssboBuffers into dependent and independent sections
//    primitiveCount should be GLTF::primitiveCount, instead of a magic number

    ssboBuffers = std::make_shared<SSBOBuffers>(device, 100);
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

    for (const std::filesystem::directory_entry& filePath : std::filesystem::recursive_directory_iterator(baseDir)) {
        // For some reason, !filePath.is_regular_file() isn't short circuiting
        // so it will try to get the file extension of a directory if these are in the same if statement
        if (filePath.exists() && filePath.is_regular_file()) {
            if ((GLTF::getFileExtension(filePath.path()).compare(".gltf") == 0) ||
                (GLTF::getFileExtension(filePath.path()).compare(".glb") == 0)) {
                futureModels.push_back(std::async(
                    std::launch::async, [filePath, fileNum, this]() { return std::make_shared<VulkanModel>(filePath.path(), fileNum, ssboBuffers); }));
                ++fileNum;
            }
        }
    }

    for (int modelIndex = 0; modelIndex < futureModels.size(); ++modelIndex) {
        std::shared_ptr<VulkanModel> model = futureModels[modelIndex].get();
        models.insert({model->model->path() + model->model->fileName(), model});
        if(model->hasAnimations()){
            animatedModels.push_back(model.get());
        }
    }

    // Load objects
    // This needs to be in a separate loop from loading models in order to dynamically size ssboBuffers
    for (std::pair<std::string, std::shared_ptr<VulkanModel>> pathModelPair : models) {
        std::string filePath = pathModelPair.first;
        std::shared_ptr<VulkanModel> model = pathModelPair.second;

        futureObjects.push_back(
            std::async(std::launch::async, [model, filePath, this]() { return new VulkanObject(model, ssboBuffers, filePath); }));
    }

    for (int objectIndex = 0; objectIndex < futureObjects.size(); ++objectIndex) {
        objects.push_back(futureObjects[objectIndex].get());
        if(objects.back()->model->hasAnimations()) {
            animatedObjects.push_back(objects.back());
        }
        std::shared_ptr<GLTF> model = objects.back()->model->model;
        std::string filePath = model->path() + model->fileName();

        // TODO
        // move this into some kind of scene file
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
            objects.back()->x(5);
            objects.back()->y(0);
            objects.back()->z(0);
            objects.back()->setScale(glm::vec3(0.1f));
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
    }

    futureObjects.clear();

    const int extraObjectCount = settings.extraObjectCount;
    const int threadCount = 10;
    const int batchSize = extraObjectCount / threadCount;
    const int extra = extraObjectCount % threadCount;
    const float randLimit = settings.randLimit;
    std::mt19937 mt(time(NULL));
    std::uniform_real_distribution<float> distribution(0, randLimit);
    srand(time(NULL));
    objects.reserve(extraObjectCount);

    std::vector<std::future<std::pair<std::vector<VulkanObject*>, std::vector<VulkanObject*>>>> futures;
    std::string filePath = baseDir + "Box.glb";
    std::shared_ptr<VulkanModel> vulkanModel = models[filePath];

    for (int batch = 0; batch < threadCount; ++batch) {
        futures.push_back(std::async(std::launch::async, [this, baseDir, vulkanModel, filePath, &distribution, &mt, randLimit, batch, batchSize, extra]() {
            std::vector<VulkanObject*> batchObjects;
            std::vector<VulkanObject*> batchAnimatedObjects;
            // NOTE:
            // adding remainder to last batch
            batchObjects.reserve(batchSize + (batch == (threadCount - 1) ? extra : 0));
            for (int objectIndex = 0; objectIndex < (batchSize + (batch == (threadCount - 1) ? extra : 0)); ++objectIndex) {
                batchObjects.push_back(new VulkanObject(vulkanModel, ssboBuffers, filePath, true));
                std::shared_ptr<GLTF> model = objects.back()->model->model;
                if (model->animations.size() > 0) {
                    batchAnimatedObjects.push_back(batchObjects.back());
                }
                float x = distribution(mt);
                float y = distribution(mt);
                float z = distribution(mt);
                batchObjects.back()->setPostion({x, y, z});
            }
            return std::pair{batchObjects, batchAnimatedObjects};
        }));
    }

    for (int i = 0; i < futures.size(); ++i) {
        std::pair<std::vector<VulkanObject*>, std::vector<VulkanObject*>> batchObjectsPair = futures[i].get();
        objects.insert(std::end(objects), std::begin(batchObjectsPair.first), std::end(batchObjectsPair.first));
        animatedObjects.insert(std::end(animatedObjects), std::begin(batchObjectsPair.second), std::end(batchObjectsPair.second));
    }

    // TODO:
    // better solution for calculating ssbo size than iterating over models and meshes twice
    uint32_t instanceCount = 0;
    for (std::pair<std::string, std::shared_ptr<VulkanModel>> model : models) {
        for (std::pair<int, std::shared_ptr<VulkanMesh>> meshPair : model.second->meshIDMap) {
            std::shared_ptr<VulkanMesh> mesh = meshPair.second;
            instanceCount += mesh->instanceIDs.size() * mesh->primitives.size();
        }
    }

    ssboBuffers->createInstanceBuffers(instanceCount);

    for (std::pair<std::string, std::shared_ptr<VulkanModel>> modelPair : models) {
        std::shared_ptr<VulkanModel> model = modelPair.second;
        for (std::pair<int, std::shared_ptr<VulkanMesh>> meshPair : model->meshIDMap) {
            std::shared_ptr<VulkanMesh> mesh = meshPair.second;
            for (std::shared_ptr<VulkanMesh::Primitive> primitive : mesh->primitives) {
                // increase size of indirect draws
                indirectDraws.resize(indirectDraws.size() + 1);

                // copy indirect draw data from primitive
                indirectDraws.back().indexCount = primitive->indices.size();
                indirectDraws.back().instanceCount = mesh->instanceIDs.size();

                // set indices and vertices
                indirectDraws.back().firstIndex = indices.size();
                indices.insert(std::end(indices), std::begin(primitive->indices), std::end(primitive->indices));

                indirectDraws.back().vertexOffset = vertices.size();
                vertices.insert(std::end(vertices), std::begin(primitive->vertices), std::end(primitive->vertices));

                // set first instance
                indirectDraws.back().firstInstance = _totalInstanceCount;

//                ssboBuffers->AABBs[_totalInstanceCount] = primitive->aabb;
                ssboBuffers->materialIndicesMapped[indirectDraws.back().firstInstance] = primitive->materialIndex;
                // TODO
                // IDs are now contiguous within a mesh, so this can be optimized
                for (uint32_t i = 0; i < mesh->instanceIDs.size(); ++i) {
                    ssboBuffers->instanceIndicesMapped[_totalInstanceCount + i] = mesh->instanceIDs[i];
                }
                _totalInstanceCount += mesh->instanceIDs.size();
            }
        }
    }

    std::for_each(std::execution::par, objects.begin(), objects.end(),[this](auto&& object) {
        object->updateModelMatrix(ssboBuffers);
    });

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

    VulkanDescriptors::VulkanDescriptor* materialDescriptor = descriptorManager->createDescriptor("material");

    materialDescriptor->addBinding(0, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, samplerInfos);
    materialDescriptor->addBinding(1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT, imageInfos);
    materialDescriptor->addBinding(2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT, normalMapInfos);
    materialDescriptor->addBinding(3, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT, metallicRoughnessMapInfos);
    materialDescriptor->addBinding(4, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT, aoMapInfos);

    materialDescriptor->allocateSets();
    materialDescriptor->update();

    VulkanDescriptors::VulkanDescriptor* objectDescriptor = descriptorManager->createDescriptor("object");

    objectDescriptor->addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, ssboBuffers->ssboBuffer());
    objectDescriptor->addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, ssboBuffers->materialBuffer());
    culledInstanceIndicesBuffer = std::make_shared<VulkanBuffer>(device, sizeof(uint32_t) * _totalInstanceCount,
                                                                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    objectDescriptor->addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, culledInstanceIndicesBuffer->buffer);
    objectDescriptor->addBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT,
                                 ssboBuffers->culledMaterialIndicesBuffer());

    objectDescriptor->allocateSets();
    objectDescriptor->update();

    VulkanDescriptors::VulkanDescriptor* cullFrustumDescriptor = descriptorManager->createDescriptor("cull_frustum_pass");

    // NOTE:
    // sorting indirect draws by firstInstance so that the draw cull pass can get the culled instance count from the prefix sum and
    // the previous draw
    std::sort(indirectDraws.begin(), indirectDraws.end(),
              [](VkDrawIndexedIndirectCommand a, VkDrawIndexedIndirectCommand b) { return a.firstInstance < b.firstInstance; });

    // NOTE: VK_BUFFER_USAGE_STORAGE_BUFFER_BIT for compute shader to read from it
    indirectDrawsBuffer =
        std::make_shared<StagedBuffer>(device, (void*)indirectDraws.data(), sizeof(indirectDraws[0]) * indirectDraws.size(),
                                       VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

    cullFrustumDescriptor->addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT,
                                      ssboBuffers->instanceIndicesBuffer());

    cullFrustumDescriptor->addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, ssboBuffers->ssboBuffer());

    prefixSumBuffer = std::make_shared<VulkanBuffer>(device, sizeof(uint32_t) * _totalInstanceCount, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    cullFrustumDescriptor->addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, prefixSumBuffer->buffer);

    partialSumsBuffer = std::make_shared<VulkanBuffer>(
        device, sizeof(uint32_t) * getGroupCount(_totalInstanceCount, device->maxComputeWorkGroupInvocations()),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    cullFrustumDescriptor->addBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, partialSumsBuffer->buffer);

    activeLanesBuffer = std::make_shared<VulkanBuffer>(device, sizeof(uint32_t) * _totalInstanceCount / 32,
                                                       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    cullFrustumDescriptor->addBinding(4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, activeLanesBuffer->buffer);

    VulkanDescriptors::VulkanDescriptor* reduceDescriptor = descriptorManager->createDescriptor("reduce_prefix_sum");

    reduceDescriptor->addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, ssboBuffers->instanceIndicesBuffer());
    reduceDescriptor->addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, prefixSumBuffer->buffer);
    reduceDescriptor->addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, partialSumsBuffer->buffer);
    reduceDescriptor->addBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, activeLanesBuffer->buffer);
    reduceDescriptor->addBinding(4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, culledInstanceIndicesBuffer->buffer);

    VulkanDescriptors::VulkanDescriptor* cullDrawDescriptor = descriptorManager->createDescriptor("cull_draw_pass");
    cullDrawDescriptor->addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, indirectDrawsBuffer->getBuffer());
    cullDrawDescriptor->addBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, prefixSumBuffer->buffer);
    cullDrawDescriptor->addBinding(4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, ssboBuffers->materialIndicesBuffer());
    cullDrawDescriptor->addBinding(5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT,
                                   ssboBuffers->culledMaterialIndicesBuffer());

    auto endTime = std::chrono::high_resolution_clock::now();
    std::cout << "Loaded " << objects.size() << " objects in "
              << std::chrono::duration<float, std::chrono::milliseconds::period>(endTime - startTime).count() << "ms" << std::endl;
}

void VulkanObjects::bind(VulkanRenderer* renderer) {
    VkBuffer vertexBuffers[] = {vertexBuffer->getBuffer()};
    VkDeviceSize offsets[] = {0};

    renderer->bindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->graphicsPipelineLayout(), 1,
                                descriptorManager->descriptors["material"]->getSets()[0]);
    renderer->bindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->graphicsPipelineLayout(), 2,
                                descriptorManager->descriptors["object"]->getSets()[0]);

    vkCmdBindVertexBuffers(renderer->getCurrentCommandBuffer(), 0, 1, vertexBuffers, offsets);

    vkCmdBindIndexBuffer(renderer->getCurrentCommandBuffer(), indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
}

void VulkanObjects::drawIndirect(VulkanRenderer* renderer) {
    for (VulkanModel* model : animatedModels) {
        model->updateAnimations();
    }
    for (VulkanObject* object : animatedObjects) {
        object->updateModelMatrix(ssboBuffers);
    }

    vkCmdDrawIndexedIndirectCount(renderer->getCurrentCommandBuffer(), renderer->culledDrawCommandsBuffer->buffer, 0,
                                  renderer->culledDrawIndirectCount->buffer, 0, indirectDraws.size(), sizeof(indirectDraws[0]));
}

VulkanObject* VulkanObjects::getObjectByName(std::string name) {
    for (VulkanObject* object : objects) {
        if (object->name().compare(name) == 0) {
            return object;
        }
    }
    throw std::runtime_error("failed to find object by name: " + name);
}

VulkanObjects::~VulkanObjects() {
    for (auto object : objects) {
        delete object;
    }
}
