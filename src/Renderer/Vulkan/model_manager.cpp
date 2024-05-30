#include "model_manager.hpp"
#include "draw.hpp"
#include <vulkan/vulkan_core.h>

ModelManager::ModelManager(DrawAllocators& draw_allocators) : _draw_allocators(draw_allocators) {}
ModelManager::~ModelManager() {
    for (auto model : _models) {
        delete model.second;
    }
}

Model* ModelManager::get_model(std::filesystem::path model_path) {
    if (_models.count(model_path) == 1) {
        return _models.at(model_path);
    } else {
        Model* model = new Model(model_path, _draw_allocators);
        _models.insert({model_path, model});
        return model;
    }
}
