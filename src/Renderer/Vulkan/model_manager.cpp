#include "model_manager.hpp"
#include "draw.hpp"
#include <vulkan/vulkan_core.h>

ModelManager::ModelManager() {}
ModelManager::~ModelManager() {
    for (auto model : _models) {
        delete model.second;
    }
}

Model* ModelManager::get_model(std::filesystem::path model_path) {
    if (_models.count(model_path) == 1) {
        return _models.at(model_path);
    } else {
        LinearAllocator<GPUAllocator>* vertex_allocator;
        LinearAllocator<GPUAllocator>* index_allocator;
        StackAllocator<GPUAllocator>* indirect_commands_allocator;
        LinearAllocator<GPUAllocator>* instance_indices_allocator;
        Model* model = new Model(model_path, vertex_allocator, index_allocator, indirect_commands_allocator, instance_indices_allocator);
        _models.insert({model_path, model});
        return model;
    }
}
