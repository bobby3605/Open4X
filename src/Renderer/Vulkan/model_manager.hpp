#ifndef MODEL_MANAGER_H_
#define MODEL_MANAGER_H_
#include "buffer.hpp"
#include "model.hpp"
#include <filesystem>

class ModelManager {
  public:
    ModelManager(Buffer* vertex_buffer, Buffer* index_buffer);
    ~ModelManager();
    Model* get_model(std::filesystem::path model_path);
    void upload_model(std::filesystem::path model_path);
    void upload_model(Model* model);

  private:
    std::unordered_map<std::filesystem::path, Model*> _models;
    Buffer* _vertex_buffer;
    Buffer* _index_buffer;
};

#endif // MODEL_MANAGER_H_
