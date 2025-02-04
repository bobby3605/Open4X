#ifndef ECS_H_
#define ECS_H_
#include <cstdint>
#include <cstring>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <vector>

// NOTE:
// https://skypjack.github.io/2019-03-07-ecs-baf-part-2/

struct MatrixComponent {
    alignas(32) glm::mat4 model_matrix;
};

struct sparse_set {
    uint32_t size = 0;
    uint32_t capacity = 0;
    uint32_t* sparse = nullptr;
    uint32_t* packed = nullptr;
    void reserve(uint32_t const& capacity);
    bool match(uint32_t const& index);
};

struct ECS {
    sparse_set entities;
    struct Component {
        uint32_t size = 0;
        uint32_t capacity = 0;
        uint32_t* sparse = nullptr;
        void* data = nullptr;
        template <typename T> void reserve(uint32_t const& new_capacity) {
            if (new_capacity > capacity) {
                if (data == nullptr) {
                    sparse = reinterpret_cast<uint32_t*>(malloc(sizeof(uint32_t) * new_capacity));
                    // NOTE:
                    // must use new instead of malloc because malloc doesn't take cpp alignas into account
                    data = new T[new_capacity];
                } else {
                    uint32_t* sparse_tmp = sparse;
                    void* data_tmp = data;
                    sparse = reinterpret_cast<uint32_t*>(malloc(sizeof(uint32_t) * new_capacity));
                    data = new T[new_capacity];
                    std::memcpy(data, data_tmp, size);
                    std::memcpy(sparse, sparse_tmp, size);
                    delete[] reinterpret_cast<T*>(data_tmp);
                    free(sparse_tmp);
                }
                capacity = new_capacity;
                // set created bytes of sparse to -1 so that it can be checked for non-existent entries
                std::memset(sparse + size, (unsigned int)(-1), capacity - size);
            }
        }
        void add_entities(std::vector<uint32_t> const& entity_ids);
    };
    // NOTE:
    // maybe should be Component*
    std::unordered_map<std::string, Component*> components;
    std::vector<uint32_t> add_entities(uint32_t const& entity_count);
    Component* register_component(std::string const& component_name);
    ~ECS();
};

#endif // ECS_H_
