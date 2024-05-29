#ifndef MODEL_MANAGER_H_
#define MODEL_MANAGER_H_
#include "model.hpp"
#include <filesystem>

class ModelManager {
  public:
    ModelManager();
    ~ModelManager();
    Model* get_model(std::filesystem::path model_path);
    void upload_model(std::filesystem::path model_path);
    void upload_model(Model* model);

  private:
    std::unordered_map<std::filesystem::path, Model*> _models;
};

#endif // MODEL_MANAGER_H_
