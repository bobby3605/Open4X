#ifndef AABB_H_
#define AABB_H_

#include <glm/glm.hpp>

class AABB {
  public:
    glm::vec3 max() { return _max; }
    glm::vec3 min() { return _min; }
    glm::vec3 length();
    glm::vec3 centerpoint();

    void update(glm::vec3 newBounds);
    void update(AABB newBounds);
    void update(glm::vec4 newBounds);

  private:
    glm::vec3 _max{-MAXFLOAT};
    glm::vec3 _min{MAXFLOAT};
    glm::vec3 _length;
    glm::vec3 _centerpoint;
    bool lengthCached = 0;
    bool centerpointCached = 0;
};

#endif // AABB_H_
