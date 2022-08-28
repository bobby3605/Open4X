#ifndef GLB_H_
#define GLB_H_
#include "JSON.hpp"
#include <string>
#include <vector>

class GLB {
public:
  GLB(std::string filePath);

private:
  static uint32_t readuint32(std::ifstream &file);
  std::vector<JSONnode> jsonChunks;
  std::vector<std::vector<unsigned char>> binaryChunks;
};

#endif // GLB_H_
