#include "vulkan_model.hpp"
#include <glm/gtc/type_ptr.hpp>

VulkanModel::VulkanModel(std::string filePath, uint32_t fileNum, std::shared_ptr<SSBOBuffers> ssboBuffers) {
    model = std::make_shared<GLTF>(filePath, fileNum);
    for (int sceneIndex = 0; sceneIndex < model->scenes.size(); ++sceneIndex) {
        rootNodes.reserve(model->scenes[sceneIndex].nodes.size());
        for (int rootNodeID : model->scenes[sceneIndex].nodes) {
            rootNodes.push_back(new VulkanNode(model, rootNodeID, &meshIDMap, &materialIDMap, _totalInstanceCounter, ssboBuffers));
        }

        for (const auto node : rootNodes) {
            // aabb is a reference to VulkanModel::aabb
            // this gets the maximum and minimum extents for the model
            node->updateAABB(glm::mat4(1.0f), aabb);
        }

        // Load animation data
        for (GLTF::Animation animation : model->animations) {
            for (std::shared_ptr<GLTF::Animation::Channel> channel : animation.channels) {
                std::shared_ptr<GLTF::Animation::Sampler> sampler = animation.samplers[channel->sampler];
                std::optional<VulkanNode*> node = findNode(channel->target->node);
                if (node.has_value()) {
                    animatedNodes.push_back(node.value());
                    node.value()->animationPair = {channel, sampler};
                }
                GLTF::Accessor* inputAccessor = &model->accessors[sampler->inputIndex];
                AccessorLoader<float> inputAccessorLoader(model.get(), inputAccessor);
                for (int count_index = 0; count_index < inputAccessor->count; ++count_index) {
                    sampler->inputData.push_back(inputAccessorLoader.at(count_index));
                }
                GLTF::Accessor* outputAccessor = &model->accessors[sampler->outputIndex];
                AccessorLoader<glm::vec4> outputAccessorLoader(model.get(), outputAccessor);
                for (int count_index = 0; count_index < outputAccessor->count; ++count_index) {
                    sampler->outputData.push_back(glm::make_mat4(glm::value_ptr(outputAccessorLoader.at(count_index))));
                }
            }
        }
    }
}

void VulkanModel::updateAnimations() {
    for (VulkanNode* node : animatedNodes) {
        node->updateAnimation();
    }
}

VulkanModel::~VulkanModel() {
    for (auto node : rootNodes) {
        delete node;
    }
}

void VulkanModel::uploadModelMatrix(uint32_t firstInstanceID, glm::mat4 modelMatrix, std::shared_ptr<SSBOBuffers> ssboBuffers) {
    for (const auto node : rootNodes) {
        node->uploadModelMatrix(firstInstanceID, modelMatrix, ssboBuffers);
    }
}

void VulkanModel::addInstance(uint32_t firstInstanceID, std::shared_ptr<SSBOBuffers> ssboBuffers) {
    for (const auto node : rootNodes) {
        node->addInstance(firstInstanceID, ssboBuffers);
    }
}

std::optional<VulkanNode*> VulkanModel::findNode(int nodeID) {
    // TODO
    // Improve this
    for (int i = 0; i < model->nodes.size(); ++i) {
        for (VulkanNode* node : rootNodes) {
            if (node->nodeID == nodeID) {
                return node;
            }
            for (VulkanNode* child : node->children) {
                if (child->nodeID == nodeID) {
                    return node;
                }
            }
        }
    }
    return std::nullopt;
}
