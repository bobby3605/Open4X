#include "model_manager.hpp"
#include "common.hpp"
#include "draw.hpp"
#include "memory_manager.hpp"
#include <vulkan/vulkan_core.h>

ModelManager::ModelManager(DrawAllocators& draw_allocators) : _draw_allocators(draw_allocators) {
    // Write default material
    _default_material_alloc = draw_allocators.material_data->alloc();
    NewMaterialData default_material{};
    _default_material_alloc->write(&default_material);
    _invalid_draws_processor = new ChunkProcessor<Draw*, safe_vector>(_invalid_draws, new_settings->invalid_draws_refresh_threads,
                                                                      [&](size_t i) { _invalid_draws[i]->write_indirect_command(); });

    _default_samplers[0] = new Sampler(1);
    MemoryManager::memory_manager->global_image_infos["samplers"].push_back(_default_samplers[0]->image_info());
}
ModelManager::~ModelManager() {
    for (auto sampler : _default_samplers) {
        delete sampler.second;
    }
    for (auto model : _models) {
        delete model.second;
    }
}

Model* ModelManager::get_model(std::filesystem::path model_path) {
    if (_models.count(model_path) == 1) {
        return _models.at(model_path);
    } else {
        Model* model = new Model(model_path, _draw_allocators, _default_material_alloc, _invalid_draws);
        _models.insert({model_path, model});
        return model;
    }
}

void ModelManager::preallocate(std::filesystem::path model_path, uint32_t count) { get_model(model_path)->preallocate(count); }

void ModelManager::refresh_invalid_draws() {
    _invalid_draws_processor->run({.offset = 0, .size = _invalid_draws.size()});
    _invalid_draws.clear();
}
