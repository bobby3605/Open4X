#include "math.hpp"
#include "../Renderer/Vulkan/common.hpp"

void AABB::update(glm::vec3 const& new_bounds) {
    _max.x = glm::max(_max.x, new_bounds.x);
    _max.y = glm::max(_max.y, new_bounds.y);
    _max.z = glm::max(_max.z, new_bounds.z);

    _min.x = glm::min(_min.x, new_bounds.x);
    _min.y = glm::min(_min.y, new_bounds.y);
    _min.z = glm::min(_min.z, new_bounds.z);

    // NOTE:
    // these are only needed if a value changes,
    // but checking that might be slower than just redoing length and centerpoint
    length_cached = false;
    centerpoint_cached = false;
}

void AABB::update(AABB const& new_bounds) {
    _max.x = glm::max(_max.x, new_bounds._max.x);
    _max.y = glm::max(_max.y, new_bounds._max.y);
    _max.z = glm::max(_max.z, new_bounds._max.z);

    _min.x = glm::min(_min.x, new_bounds._min.x);
    _min.y = glm::min(_min.y, new_bounds._min.y);
    _min.z = glm::min(_min.z, new_bounds._min.z);

    length_cached = false;
    centerpoint_cached = false;
}

void AABB::update(glm::vec4 const& new_bounds) { update(glm::vec3(new_bounds)); }

glm::vec3 AABB::length() {
    if (!length_cached) {
        _length = {_max.x - _min.x, _max.y - _min.y, _max.z - _min.z};
        length_cached = true;
    }
    return _length;
}

glm::vec3 AABB::centerpoint() {
    if (!centerpoint_cached) {
        _centerpoint = {_max.x - length().x / 2, _max.y - length().y / 2, _max.z - length().z / 2};
        centerpoint_cached = true;
    }
    return _centerpoint;
}

OBB AABB::toOBB(glm::mat4 const& trs) {
    // https://bruop.github.io/improved_frustum_culling/
    glm::vec3 corners[] = {
        {_min.x, _min.y, _min.z},
        {_max.x, _min.y, _min.z},
        {_min.x, _max.y, _min.z},
        {_min.x, _min.y, _max.z},
    };
    for (size_t corner_idx = 0; corner_idx < 4; corner_idx++) {
        corners[corner_idx] = trs * glm::vec4(corners[corner_idx], 1.0f);
    }
    OBB obb;
    obb.directionU = corners[1] - corners[0];
    obb.directionV = corners[2] - corners[0];
    obb.directionW = corners[3] - corners[0];
    obb.center = corners[0] + 0.5f * (obb.directionU + obb.directionV + obb.directionW);
    obb.half_extents = {glm::length(obb.directionU), glm::length(obb.directionV), glm::length(obb.directionW)};
    obb.directionU = obb.directionU / obb.half_extents.x;
    obb.directionV = obb.directionV / obb.half_extents.y;
    obb.directionW = obb.directionW / obb.half_extents.z;
    obb.half_extents *= 0.5f;
    return obb;
}
