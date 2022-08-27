#include "GLB.hpp"
#include <fstream>
#include <iostream>

uint32_t GLB::readuint32(std::ifstream &file) {
  uint32_t buffer;
  file.read((char *)&buffer, sizeof(uint32_t));
  return buffer;
}

GLB::Chunk::Chunk(std::ifstream &file) {
  _chunkLength = GLB::readuint32(file);
  _chunkType = GLB::readuint32(file);
  unsigned char dataBuffer;
  for (int i = 0; i < _chunkLength; i += sizeof(dataBuffer)) {
    file.read((char *)&dataBuffer, sizeof(dataBuffer));
    chunkData.push_back(dataBuffer);
  }
}

void GLB::Chunk::print() {
  std::cout << "Chunk Length: " << chunkLength() << std::endl
            << "Chunk Type: " << chunkType() << std::endl
            << "Chunk Data: " << std::endl;
  for (auto byte : chunkData) {
    std::cout << byte;
  }
  std::cout << std::endl;
}

GLB::GLB(std::string filePath) {
  std::ifstream file(filePath.c_str(), std::ios::binary);
  uint32_t magic = readuint32(file);
  uint32_t version = readuint32(file);
  uint32_t length = readuint32(file);
  std::cout << "Magic: " << magic << std::endl
            << "Version: " << version << std::endl
            << "Length: " << length << std::endl;
  while (file.tellg() < length) {
    chunks.push_back(Chunk(file));
    chunks.back().print();
  }
  file.close();
}
