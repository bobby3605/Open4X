#include "model_manager.hpp"
#include <vulkan/vulkan_core.h>

ModelManager::ModelManager(Buffer* vertex_buffer, Buffer* index_buffer) : _vertex_buffer{vertex_buffer}, _index_buffer{index_buffer} {}
ModelManager::~ModelManager() {
    for (auto model : _models) {
        delete model.second;
    }
}

Model* ModelManager::get_model(std::filesystem::path model_path) {
    if (_models.count(model_path) == 1) {
        return _models.at(model_path);
    } else {
        Model* model = new Model(model_path);
        _models.insert({model_path, model});
        return model;
    }
}

void ModelManager::upload_model(std::filesystem::path model_path) { upload_model(get_model(model_path)); }
void ModelManager::upload_model(Model* model) { /*model->alloc(_vertex_buffer, _index_buffer); */
}
