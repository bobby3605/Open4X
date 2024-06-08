#include "model_manager.hpp"
#include "draw.hpp"
#include <vulkan/vulkan_core.h>

ModelManager::ModelManager(DrawAllocators& draw_allocators) : _draw_allocators(draw_allocators) {
    // Write default material
    _default_material_alloc = draw_allocators.material_data->alloc();
    NewMaterialData default_material{};
    _default_material_alloc->write(&default_material);
}
ModelManager::~ModelManager() {
    for (auto model : _models) {
        delete model.second;
    }
}

Model* ModelManager::get_model(std::filesystem::path model_path) {
    if (_models.count(model_path) == 1) {
        return _models.at(model_path);
    } else {
        Model* model = new Model(model_path, _draw_allocators, _default_material_alloc);
        _models.insert({model_path, model});
        return model;
    }
}
