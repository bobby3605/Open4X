#include "math.hpp"

void NewAABB::update(glm::vec3 const& new_bounds) {
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

void NewAABB::update(NewAABB const& new_bounds) {
    _max.x = glm::max(_max.x, new_bounds._max.x);
    _max.y = glm::max(_max.y, new_bounds._max.y);
    _max.z = glm::max(_max.z, new_bounds._max.z);

    _min.x = glm::min(_min.x, new_bounds._min.x);
    _min.y = glm::min(_min.y, new_bounds._min.y);
    _min.z = glm::min(_min.z, new_bounds._min.z);

    length_cached = false;
    centerpoint_cached = false;
}

void NewAABB::update(glm::vec4 const& new_bounds) { update(glm::vec3(new_bounds)); }

glm::vec3 NewAABB::length() {
    if (!length_cached) {
        _length = {_max.x - _min.x, _max.y - _min.y, _max.z - _min.z};
        length_cached = true;
    }
    return _length;
}

glm::vec3 NewAABB::centerpoint() {
    if (!centerpoint_cached) {
        _centerpoint = {_max.x - length().x / 2, _max.y - length().y / 2, _max.z - length().z / 2};
        centerpoint_cached = true;
    }
    return _centerpoint;
}
