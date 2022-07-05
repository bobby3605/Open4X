#include "Vulkan/vulkan_descriptors.hpp"
#include <vector>
#include <vulkan/vulkan_core.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "open4x.hpp"
#include "Vulkan/common.hpp"
#include "Vulkan/vulkan_buffer.hpp"
#include "Vulkan/vulkan_object.hpp"
#include "Vulkan/vulkan_swapchain.hpp"
#include <GLFW/glfw3.h>
#include <chrono>
#include <cstring>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, 1);
  }
}

glm::mat4 perspectiveProjection(float vertical_fov, float aspect_ratio, float near, float far) {
  assert(glm::abs(aspect_ratio - std::numeric_limits<float>::epsilon()) > 0.0f);
  const float tanHalfFovy = tan(glm::radians(vertical_fov) / 2.f);
  glm::mat4 projectionMatrix{0.0f};
  projectionMatrix[0][0] = 1.f / (aspect_ratio * tanHalfFovy);
  projectionMatrix[1][1] = 1.f / (tanHalfFovy);
  projectionMatrix[2][2] = far / (far - near);
  projectionMatrix[2][3] = 1.f;
  projectionMatrix[3][2] = -(far * near) / (far - near);
  return projectionMatrix;
}

Open4X::Open4X() {
  vulkanWindow = new VulkanWindow(640, 480, "Open 4X");
  glfwSetKeyCallback(vulkanWindow->getGLFWwindow(), key_callback);

  vulkanDevice = new VulkanDevice(vulkanWindow);

  vikingRoomModel = new VulkanModel(vulkanDevice, "assets/models/viking_room.obj");
  flatVaseModel = new VulkanModel(vulkanDevice, "assets/models/flat_vase.obj");
}

Open4X::~Open4X() {
  delete vikingRoomModel;
  delete flatVaseModel;
  delete vulkanRenderer;
  delete vulkanDevice;
  delete vulkanWindow;
}

void Open4X::run() {

  VulkanDescriptors descriptorManager(vulkanDevice);

  VkDescriptorPool pool = descriptorManager.createPool();
  VkDescriptorSetLayout globalL = descriptorManager.createLayout(descriptorManager.globalLayout());
  VkDescriptorSetLayout materialL = descriptorManager.createLayout(descriptorManager.materialLayout());

  std::vector<VkDescriptorSetLayout> descriptorLayouts;
  descriptorLayouts.push_back(globalL);
  descriptorLayouts.push_back(materialL);

  vulkanRenderer = new VulkanRenderer(vulkanWindow, vulkanDevice, descriptorLayouts);

  std::vector<VkDescriptorSet> globalSets;
  std::vector<VkDescriptorSet> obj1MaterialSets;
  std::vector<VkDescriptorSet> obj2MaterialSets;

  descriptorManager.createSets(pool, globalL, globalSets);
  descriptorManager.createSets(pool, materialL, obj1MaterialSets);
  descriptorManager.createSets(pool, materialL, obj2MaterialSets);

  std::vector<UniformBuffer *> uniformBuffers(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);
  std::vector<VkDescriptorBufferInfo> bufferInfos(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);

  for (int i = 0; i < VulkanSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
    uniformBuffers[i] = new UniformBuffer(vulkanDevice);
    bufferInfos[i] = uniformBuffers[i]->getBufferInfo();

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = globalSets[i];
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfos[i];

    vkUpdateDescriptorSets(vulkanDevice->device(), 1, &descriptorWrite, 0, nullptr);
  }

  VkSampler vikingRoomSampler;
  VkImageView vikingRoomImageView;
  vulkanRenderer->loadImage("assets/textures/viking_room.png", vikingRoomSampler, vikingRoomImageView);

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = vikingRoomImageView;
    imageInfo.sampler = vikingRoomSampler;

    VkWriteDescriptorSet descriptorWrite{};

  for (int i = 0; i < VulkanSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = obj1MaterialSets[i];
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(vulkanDevice->device(), 1, &descriptorWrite, 0,
                           nullptr);
  }

  vulkanRenderer->loadImage("assets/textures/statue.jpg", vikingRoomSampler, vikingRoomImageView);
    imageInfo.imageView = vikingRoomImageView;
    imageInfo.sampler = vikingRoomSampler;

  for (int i = 0; i < VulkanSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = obj2MaterialSets[i];
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(vulkanDevice->device(), 1, &descriptorWrite, 0,
                           nullptr);
  }

  //vulkanRenderer->createDescriptorSets(bufferInfos);

  VulkanObject obj1(vikingRoomModel, vulkanRenderer);
  VulkanObject obj2(flatVaseModel, vulkanRenderer);
  VulkanObject obj3(flatVaseModel, vulkanRenderer);
  camera = new VulkanObject(vulkanRenderer);

  UniformBufferObject ubo{};

  auto startTime = std::chrono::high_resolution_clock::now();
  while (!glfwWindowShouldClose(vulkanWindow->getGLFWwindow())) {
    glfwPollEvents();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(startTime - currentTime).count();
    startTime = currentTime;
    camera->keyboardUpdate(frameTime);

    ubo.view = glm::inverse(camera->mat4());
    // TODO cache projection matrix
    ubo.proj = perspectiveProjection(
        45.0f, vulkanRenderer->getSwapChainExtent().width / (float)vulkanRenderer->getSwapChainExtent().height, 0.001f,
        100.0f);

    vulkanRenderer->startFrame();

    uniformBuffers[vulkanRenderer->getCurrentFrame()]->write(&ubo);

    vulkanRenderer->beginSwapChainrenderPass();
    vulkanRenderer->bindPipeline();

  //  vulkanRenderer->bindDescriptorSets();

    vulkanRenderer->bindDescriptorSet(0, globalSets[vulkanRenderer->getCurrentFrame()]);

    vulkanRenderer->bindDescriptorSet(1, obj1MaterialSets[vulkanRenderer->getCurrentFrame()]);
    obj1.draw();
    obj2.position.y = 1.5f;
    vulkanRenderer->bindDescriptorSet(1, obj2MaterialSets[vulkanRenderer->getCurrentFrame()]);
    obj2.draw();

    vulkanRenderer->endSwapChainrenderPass();

    vulkanRenderer->endFrame();
  }
  vkDeviceWaitIdle(vulkanDevice->device());
  delete camera;
  for (size_t i = 0; i < VulkanSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
    delete uniformBuffers[i];
  }
}
