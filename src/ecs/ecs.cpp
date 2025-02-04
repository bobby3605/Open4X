#include "ecs.hpp"
#include <cstdlib>
#include <cstring>

void sparse_set::reserve(uint32_t const& total_capacity) {
    if (capacity < total_capacity) {
        uint32_t new_byte_size = total_capacity * sizeof(uint32_t);
        uint32_t old_byte_size = size * sizeof(uint32_t);
        uint32_t* new_sparse = reinterpret_cast<uint32_t*>(std::malloc(new_byte_size));
        uint32_t* new_packed = reinterpret_cast<uint32_t*>(std::malloc(new_byte_size));
        // NOTE:
        // It's technically safer to check sparse and packed individually,
        // but if one is null and the other isn't, then that is undefined behavior
        if (sparse != nullptr) {
            std::memcpy(new_sparse, sparse, old_byte_size);
            free(sparse);
            std::memcpy(new_packed, packed, old_byte_size);
            free(packed);
        }
        sparse = new_sparse;
        packed = new_packed;
    }
}

bool sparse_set::match(uint32_t const& index) { return packed[sparse[index]] == index; }

// FIXME:
// delete_entities
std::vector<uint32_t> ECS::add_entities(uint32_t const& entity_count) {
    std::vector<uint32_t> out(entity_count);
    uint32_t base_offset = entities.size;
    entities.reserve(entities.size + entity_count);
    entities.size += entity_count;
    // TODO:
    // move this into reserve
    for (uint32_t i = base_offset; i < entities.size; ++i) {
        entities.packed[i] = i;
        // FIXME:
        // handle deleted nodes
        entities.sparse[i] = i;
        out[i] = i;
    }
    return out;
}

ECS::Component* ECS::register_component(std::string const& component_name) {
    Component* c = new Component;
    components.insert({component_name, c});
    return c;
}

ECS::~ECS() {
    for (auto& c : components) {
        delete c.second;
    }
}

void ECS::Component::add_entities(std::vector<uint32_t> const& entity_ids) {
    for (auto const& id : entity_ids) {
        sparse[id] = size++;
    }
}
