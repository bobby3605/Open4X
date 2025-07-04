#include <cstring>
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan_core.h>

struct InstanceData {
    glm::mat4 model_matrix;
};

std::vector<InstanceData> instances;

struct Primitive {
    VkDrawIndexedIndirectCommand cmd;
    std::vector<InstanceData> model_matrices;
};

struct Model {
    std::vector<Primitive> primitives;
};

std::vector<Model> models;

void add_model_instances(std::unordered_map<Model*, size_t> const& instance_counts) {}
