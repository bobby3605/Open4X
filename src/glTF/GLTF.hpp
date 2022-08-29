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

class GLTF {
public:
  GLTF(std::string filePath);
  ~GLTF();

private:
  std::string getFileExtension(std::string filePath);
  void loadURI(std::string uri, int byteLength);
  void loadGLTF(std::string filePath);
  uint32_t readuint32(std::ifstream &file);
  void loadGLB(std::string filePath);
  JSONnode *jsonRoot = nullptr;
  std::vector<std::vector<unsigned char>> buffers;
};

#endif // GLTF_H_
