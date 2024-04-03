#include "aabb.hpp"
#include "common.hpp"
#include <glm/gtx/string_cast.hpp>

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

OBB AABB::toOBB(glm::quat rotation, glm::vec3 scale) {
    OBB obb;
    obb.center = ((max() + min()) * 0.5f) * scale;
    obb.half_extents = ((max() - min()) * 0.5f) * scale;
    obb.directionU = rotation * rightVector;
    obb.directionV = rotation * upVector;
    // FIXME:
    // Get rid of the * -1
    // upVector is in -y space
    // but rotation is in +y space
    // this converts directionV to +y space,
    // which is used for frustum culling calculations (I think)
    // Ideally, the rotation quaternion would be directly passed to frustum culling
    obb.directionV *= -1;
    obb.directionW = rotation * forwardVector;
    return obb;
}
