#include "aabb.hpp"

void AABB::update(glm::vec3 newBounds) {
    _max.x = glm::max(_max.x, newBounds.x);
    _max.y = glm::max(_max.y, newBounds.y);
    _max.z = glm::max(_max.z, newBounds.z);

    _min.x = glm::min(_min.x, newBounds.x);
    _min.y = glm::min(_min.y, newBounds.y);
    _min.z = glm::min(_min.z, newBounds.z);

    // NOTE:
    // these are only needed if a value changes,
    // but checking that might be slower than just redoing length and centerpoint
    lengthCached = 0;
    centerpointCached = 0;
}

void AABB::update(AABB newBounds) {
    _max.x = glm::max(_max.x, newBounds._max.x);
    _max.y = glm::max(_max.y, newBounds._max.y);
    _max.z = glm::max(_max.z, newBounds._max.z);

    _min.x = glm::min(_min.x, newBounds._min.x);
    _min.y = glm::min(_min.y, newBounds._min.y);
    _min.z = glm::min(_min.z, newBounds._min.z);

    lengthCached = 0;
    centerpointCached = 0;
}

void AABB::update(glm::vec4 newBounds) { update(glm::vec3(newBounds)); }

glm::vec3 AABB::length() {
    if (!lengthCached) {
        _length = {_max.x - _min.x, _max.y - _min.y, _max.z - _min.z};
        lengthCached = 1;
    }
    return _length;
}

glm::vec3 AABB::centerpoint() {
    if (!centerpointCached) {
        _centerpoint = {_max.x - length().x / 2, _max.y - length().y / 2, _max.z - length().z / 2};
        centerpointCached = 1;
    }
    return _centerpoint;
}

OBB AABB::toOBB(glm::quat rotation) {
    OBB obb;
    obb.center = (max() + min()) * 0.5f;
    obb.half_extents = (max() - min()) * 0.5f;
    // TODO
    // directions might need to be normalized, but probably not
    obb.directionU = rotation * glm::vec3(1.0f, 0.0f, 0.0f);
    obb.directionV = rotation * glm::vec3(0.0f, -1.0f, 0.0f);
    obb.directionW = rotation * glm::vec3(0.0f, 0.0f, 1.0f);
    return obb;
}
