// DO NOT USE
// USE GLB FILES FOR NOW
#ifndef GLTF_H_
#define GLTF_H_

#include "JSON.hpp"
#include <glm/glm.hpp>
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <unordered_map>
#include <vector>

// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#properties-reference

namespace gltf {

class GLTF {
public:
  GLTF(std::string filePath);
};

} // namespace gltf
#endif // GLTF_H_
