#include "model_manager.hpp"
#include "common.hpp"
#include "draw.hpp"
#include <vulkan/vulkan_core.h>

ModelManager::ModelManager(DrawAllocators& draw_allocators) : _draw_allocators(draw_allocators) {
    // Write default material
    _default_material_alloc = draw_allocators.material_data->alloc();
    NewMaterialData default_material{};
    _default_material_alloc->write(&default_material);

    size_t num_threads = std::min((unsigned int)new_settings->indirect_write_threads, std::thread::hardware_concurrency());
    _invalid_draws_slicer =
        new VectorSlicer<Draw*>(_invalid_draws, num_threads, [&](size_t i) { _invalid_draws[i]->write_indirect_command(); });
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
        Model* model = new Model(model_path, _draw_allocators, _default_material_alloc, _invalid_draws, _invalid_draws_idx);
        _models.insert({model_path, model});
        return model;
    }
}

void ModelManager::preallocate(std::filesystem::path model_path, uint32_t count) { get_model(model_path)->preallocate(count); }

void ModelManager::refresh_invalid_draws() {
    // NOTE:
    // _invalid_draws_idx uses pre-increment, so this is the correct size
    _invalid_draws_slicer->run({.offset = 0, .size = _invalid_draws_idx});
    _invalid_draws_idx = 0;
}
