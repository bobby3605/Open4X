#ifndef AABB_H_
#define AABB_H_

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

struct OBB {
    glm::vec3 center;
    uint32_t pad;
    glm::vec3 half_extents;
    uint32_t pad1;
    glm::vec3 directionU;
    uint32_t pad2;
    glm::vec3 directionV;
    uint32_t pad3;
    glm::vec3 directionW;
    uint32_t pad4;
};

class AABB {
  public:
    glm::vec3 max() { return _max; }
    glm::vec3 min() { return _min; }
    glm::vec3 length();
    glm::vec3 centerpoint();

    void update(glm::vec3 newBounds);
    void update(AABB newBounds);
    void update(glm::vec4 newBounds);

    OBB toOBB(glm::quat rot);

  private:
    glm::vec3 _max{-MAXFLOAT};
    glm::vec3 _min{MAXFLOAT};
    glm::vec3 _length;
    glm::vec3 _centerpoint;
    bool lengthCached = 0;
    bool centerpointCached = 0;
};

#endif // AABB_H_
