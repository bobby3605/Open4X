#include "object.hpp"
#include <entt/entt.hpp>

void create_objects(entt::registry& registry, size_t const& count) {
    std::vector<entt::entity> objects(count);
    registry.create(objects.begin(), objects.end());

    registry.insert<Position>(objects.begin(), objects.end(), {0.0f, 0.0f, 0.0f});
    registry.insert<Rotation>(objects.begin(), objects.end(), {1.0f, 0.0f, 0.0f, 0.0f});
    registry.insert<Scale>(objects.begin(), objects.end(), {1.0f, 1.0f, 1.0f});
    registry.insert<ObjectMatrix>(objects.begin(), objects.end(), {1.0f});
}
