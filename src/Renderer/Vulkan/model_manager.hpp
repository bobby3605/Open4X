#ifndef MODEL_MANAGER_H_
#define MODEL_MANAGER_H_
#include "draw.hpp"
#include "model.hpp"
#include <filesystem>

class ModelManager {
  public:
    ModelManager(DrawAllocators& draw_allocators);
    ~ModelManager();
    Model* get_model(std::filesystem::path model_path);
    void preallocate(std::filesystem::path model_path, uint32_t count);
    void refresh_invalid_draws();

  private:
    std::unordered_map<std::filesystem::path, Model*> _models;
    DrawAllocators _draw_allocators;
    SubAllocation<FixedAllocator, GPUAllocation>* _default_material_alloc;
    safe_vector<Draw*> _invalid_draws;
    ChunkProcessor<Draw*, safe_vector>* _invalid_draws_processor;
    std::unordered_map<uint32_t, Sampler*> _default_samplers;
    std::unordered_map<uint32_t, Image*> _default_images;
};

#endif // MODEL_MANAGER_H_
