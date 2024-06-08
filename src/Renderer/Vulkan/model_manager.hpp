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

  private:
    std::unordered_map<std::filesystem::path, Model*> _models;
    DrawAllocators _draw_allocators;
    SubAllocation<FixedAllocator, GPUAllocation>* _default_material_alloc;
};

#endif // MODEL_MANAGER_H_
