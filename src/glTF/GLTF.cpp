#include "GLTF.hpp"

using namespace gltf;
GLTF::GLTF(std::string filePath) {
  std::ifstream file(filePath);
  JSONnode root = JSONnode(file);
  file.close();
}
