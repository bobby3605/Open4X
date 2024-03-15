#include "vulkan_objects.hpp"
#include "../glTF/base64.hpp"
#include "common.hpp"
#include "vulkan_buffer.hpp"
#include "vulkan_descriptors.hpp"
#include "vulkan_device.hpp"
#include "vulkan_image.hpp"
#include "vulkan_node.hpp"
#include "vulkan_object.hpp"
#include "vulkan_rendergraph.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <execution>
#include <filesystem>
#include <glm/gtx/string_cast.hpp>
#include <iterator>
#include <memory>
#include <pstl/glue_execution_defs.h>
#include <random>
#include <vulkan/vulkan_core.h>

VulkanObjects::VulkanObjects(std::shared_ptr<VulkanDevice> device, VulkanRenderGraph* rg, std::shared_ptr<Settings> settings)
    : device{device} {
    _totalInstanceCount = 0;
    auto startTime = std::chrono::high_resolution_clock::now();
    // Load models
    uint32_t fileNum = 0;
    const std::string baseDir = "assets/glTF/";

    ssboBuffers = std::make_shared<SSBOBuffers>(device);
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
            if ((getFileExtension(filePath.path()).compare("gltf") == 0) || (getFileExtension(filePath.path()).compare("glb") == 0)) {
                futureModels.push_back(std::async(std::launch::async, [filePath, fileNum, this]() {
                    return std::make_shared<VulkanModel>(filePath.path(), fileNum, ssboBuffers);
                }));
                ++fileNum;
            }
        }
    }

    for (int modelIndex = 0; modelIndex < futureModels.size(); ++modelIndex) {
        std::shared_ptr<VulkanModel> model = futureModels[modelIndex].get();
        models.insert({model->model->path() + model->model->fileName(), model});
        if (model->hasAnimations()) {
            animatedModels.push_back(model.get());
        }
    }

    // NOTE:
    // Creating material buffer after all gltf files have been loaded
    // TODO:
    // Better solution, possibly dynamically creating and resizing material buffer
    ssboBuffers->createMaterialBuffer(GLTF::primitiveCount);

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
        if (objects.back()->model->hasAnimations()) {
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

    const int extraObjectCount = settings->extraObjectCount;
    const int threadCount = 10;
    const int batchSize = extraObjectCount / threadCount;
    const int extra = extraObjectCount % threadCount;
    const float randLimit = settings->randLimit;
    std::mt19937 mt(time(NULL));
    std::uniform_real_distribution<float> distribution(0, randLimit);
    srand(time(NULL));
    objects.reserve(extraObjectCount);

    std::vector<std::future<std::pair<std::vector<VulkanObject*>, std::vector<VulkanObject*>>>> futures;
    std::string filePath = baseDir + "Box.glb";
    std::shared_ptr<VulkanModel> vulkanModel = models[filePath];

    for (int batch = 0; batch < threadCount; ++batch) {
        futures.push_back(
            std::async(std::launch::async, [this, baseDir, vulkanModel, filePath, &distribution, &mt, batch, batchSize, extra]() {
                std::vector<VulkanObject*> batchObjects;
                std::vector<VulkanObject*> batchAnimatedObjects;
                // NOTE:
                // adding remainder to last batch
                batchObjects.reserve(batchSize + (batch == (threadCount - 1) ? extra : 0));
                for (int objectIndex = 0; objectIndex < (batchSize + (batch == (threadCount - 1) ? extra : 0)); ++objectIndex) {
                    batchObjects.push_back(new VulkanObject(vulkanModel, ssboBuffers, filePath));
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
        // FIXME:
        // batchAnimatedObjects
        animatedObjects.insert(std::end(animatedObjects), std::begin(batchObjectsPair.second), std::end(batchObjectsPair.second));
    }

    // TODO:
    // better solution for calculating ssbo size than iterating over models and meshes twice
    uint32_t instanceCount = 0;
    for (std::pair<std::string, std::shared_ptr<VulkanModel>> model : models) {
        for (std::pair<int, std::shared_ptr<VulkanMesh>> meshPair : model.second->meshIDMap) {
            std::shared_ptr<VulkanMesh> mesh = meshPair.second;
            instanceCount += mesh->instanceIDs.size() * mesh->primitives.size();
            // NOTE:
            // Uploading materials after material buffer has been generated
            // Material buffer needs to have a size in order to be generated
            // So I need to know how many unique materials I have first
            for (const auto& primitive : mesh->primitives) {
                primitive->uploadMaterial(ssboBuffers);
            }
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

    // NOTE:
    // sorting indirect draws by firstInstance so that the draw cull pass can get the culled instance count from the prefix sum and
    // the previous draw
    std::sort(indirectDraws.begin(), indirectDraws.end(),
              [](VkDrawIndexedIndirectCommand a, VkDrawIndexedIndirectCommand b) { return a.firstInstance < b.firstInstance; });

    objectCullingData = VulkanBuffer::StorageBuffer(device, objects.size() * sizeof(ObjectCullData));
    ObjectCullData* objectCullingDataMapped = reinterpret_cast<ObjectCullData*>(objectCullingData->mapped());

    std::atomic<int32_t> i = 0;
    std::for_each(std::execution::par_unseq, objects.begin(), objects.end(), [this, &i, objectCullingDataMapped](auto&& object) {
        int32_t index = i.fetch_add(1);
        object->updateModelMatrix(ssboBuffers);
        // FIXME:
        // Do this whenever the object changes
        // Objects can be in random order, as long as parameters are set properly
        // FIXME:
        // There's some issue with instance IDs
        objectCullingDataMapped[index].obb = object->obb;
        objectCullingDataMapped[index].firstInstanceID = object->firstInstanceID;
        objectCullingDataMapped[index].instanceCount = object->model->totalInstanceCount();
    });

    vertexBuffer = VulkanBuffer::StagedBuffer(device, (void*)vertices.data(), sizeof(vertices[0]) * vertices.size(),
                                              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    indexBuffer =
        VulkanBuffer::StagedBuffer(device, (void*)indices.data(), sizeof(indices[0]) * indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    // Get unique samplers and load into continuous vector
    samplerInfos.resize(ssboBuffers->uniqueSamplersMap.size());
    for (auto it = ssboBuffers->uniqueSamplersMap.begin(); it != ssboBuffers->uniqueSamplersMap.end(); ++it) {
        samplerInfos[it->second].sampler = reinterpret_cast<VulkanSampler*>(it->first)->imageSampler();
        samplerInfos[it->second].imageView = VK_NULL_HANDLE;
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

    rg->buffer("DrawCommands", indirectDraws, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
        .buffer("CulledMaterialIndices", ssboBuffers->culledMaterialIndicesBuffer())
        .buffer("MaterialIndices", ssboBuffers->materialIndicesBuffer())
        .buffer("CulledDrawCommands", indirectDraws.size(), VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, 0)
        .buffer("CulledDrawIndirectCount", 1, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, 0)
        .buffer("CulledInstanceIndices", _totalInstanceCount)
        .buffer("Globals", 1)
        .buffer("Materials", ssboBuffers->materialBuffer())
        .buffer("Instances", ssboBuffers->instanceIndicesBuffer())
        .buffer("Objects", ssboBuffers->ssboBuffer())
        .buffer("PrefixSum", _totalInstanceCount)
        .buffer("PartialSums", getGroupCount(_totalInstanceCount, device->maxComputeWorkGroupInvocations()),
                VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        // NOTE:
        // / 32 because ballot returns a uvec4, each uint in the vector is 32 bits long
        // adding 32 - 1 ensures correct rounding
        .buffer("ActiveLanes", (_totalInstanceCount + (32 - 1)) / 32)
        .buffer("ObjectCullingData", objectCullingData)
        .buffer("VisibilityBuffer", _totalInstanceCount);

    const uint32_t queryCount = 4;
    VkQueryPoolCreateInfo queryPoolInfo{};
    queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    queryPoolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
    queryPoolInfo.queryCount = queryCount;

    vkCreateQueryPool(device->device(), &queryPoolInfo, nullptr, &queryPool);
    vkResetQueryPool(device->device(), queryPool, 0, queryCount);

    rg->queryReset(queryPool, 0, queryCount);

    VulkanRenderGraph::ShaderOptions shaderOptions{};

    specData.local_size_x = device->maxComputeWorkGroupInvocations();
    specData.subgroup_size = device->maxSubgroupSize();
    shaderOptions.specData = &specData;

    rg->timestamp(VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, queryPool, 0);
    // ensure previous frame vertex read completed before writing
    rg->memoryBarrier(VK_ACCESS_2_SHADER_STORAGE_READ_BIT, VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
                      VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);

    frustumCullPushConstants.totalObjectCount = objects.size();
    shaderOptions.pushConstantData = &frustumCullPushConstants;
    rg->shader("frustum_cull_objects.comp", getGroupCount(objects.size(), device->maxComputeWorkGroupInvocations()), 1, 1, shaderOptions);
    //  ensure frustum_cull_objects write completed
    rg->memoryBarrier(VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
                      VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
    shaderOptions.pushConstantData = &_totalInstanceCount;

    rg->shader("prefix_sum_first_pass.comp", getGroupCount(_totalInstanceCount, device->maxComputeWorkGroupInvocations()), 1, 1,
               shaderOptions);
    // wait until the frustum culling is done
    rg->memoryBarrier(VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
                      VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);

    rg->shader("reduce_prefix_sum.comp", getGroupCount(_totalInstanceCount, device->maxComputeWorkGroupInvocations()), 1, 1, shaderOptions);
    // wait until finished
    rg->memoryBarrier(VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
                      VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);

    // ensure previous frame vertex read completed before zeroing out buffer
    rg->memoryBarrier(VK_ACCESS_2_SHADER_STORAGE_READ_BIT, VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
                      VK_PIPELINE_STAGE_2_COPY_BIT);

    rg->setBuffer("CulledDrawIndirectCount", 0);

    drawCount = indirectDraws.size();
    shaderOptions.pushConstantData = &drawCount;
    // barrier until the CulledDrawIndirectCount buffer has been cleared
    rg->memoryBarrier(VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_COPY_BIT,
                      VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);

    rg->shader("cull_draw_pass.comp", getGroupCount(drawCount, device->maxComputeWorkGroupInvocations()), 1, 1, shaderOptions);
    rg->timestamp(VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, queryPool, 1);

    // wait until culling is completed
    rg->memoryBarrier(VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                      VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_READ_BIT, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT);

    VulkanRenderGraph::ShaderOptions vertOptions{};
    VulkanRenderGraph::ShaderOptions fragOptions{};

    rg->shader("triangle.vert", "triangle.frag", vertOptions, fragOptions, vertexBuffer, indexBuffer);

    rg->imageInfos("samplers", &samplerInfos);
    rg->imageInfos("images", &imageInfos);
    rg->imageInfos("normals", &normalMapInfos);
    rg->imageInfos("metallicRoughnesses", &metallicRoughnessMapInfos);
    rg->imageInfos("aos", &aoMapInfos);

    rg->timestamp(VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, queryPool, 2);
    rg->drawIndirect("CulledDrawCommands", 0, "CulledDrawIndirectCount", 0, indirectDraws.size(), sizeof(indirectDraws[0]));
    rg->timestamp(VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, queryPool, 3);
    rg->compile();

    auto endTime = std::chrono::high_resolution_clock::now();
    std::cout << "Loaded " << objects.size() << " objects in "
              << std::chrono::duration<float, std::chrono::milliseconds::period>(endTime - startTime).count() << "ms" << std::endl;
}

void VulkanObjects::updateModels() {
    for (VulkanModel* model : animatedModels) {
        model->updateAnimations();
    }
    for (VulkanObject* object : animatedObjects) {
        object->updateModelMatrix(ssboBuffers);
    }
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
    vkDestroyQueryPool(device->device(), queryPool, nullptr);
    for (auto object : objects) {
        delete object;
    }
}
