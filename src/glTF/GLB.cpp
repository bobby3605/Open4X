#include "GLB.hpp"
#include <fstream>
#include <iostream>

uint32_t GLB::readuint32(std::ifstream &file) {
  uint32_t buffer;
  file.read((char *)&buffer, sizeof(uint32_t));
  return buffer;
}

GLB::GLB(std::string filePath) {
  std::ifstream file(filePath.c_str(), std::ios::binary);
  uint32_t magic = readuint32(file);
  uint32_t version = readuint32(file);
  uint32_t length = readuint32(file);
  while (file.tellg() < length) {
    // Get chunk info
    uint32_t chunkLength = GLB::readuint32(file);
    uint32_t chunkType = GLB::readuint32(file);

    unsigned char dataBuffer;
    if (chunkType == 0x4E4F534A) {
      // JSON chunk
      std::stringstream jsonString;
      for (int i = 0; i < chunkLength; i += sizeof(dataBuffer)) {
        file.read((char *)&dataBuffer, sizeof(dataBuffer));
        jsonString << dataBuffer;
      }
      jsonChunks.push_back(JSONnode(jsonString));
      // Reset JSON stringstream
      jsonString.str("");
      jsonString.clear();

    } else if (chunkType == 0x004E4942) {
      // Binary chunk
      std::vector<unsigned char> binaryChunk;
      for (int i = 0; i < chunkLength; i += sizeof(dataBuffer)) {
        file.read((char *)&dataBuffer, sizeof(dataBuffer));
        binaryChunk.push_back(dataBuffer);
      }
      binaryChunks.push_back(binaryChunk);
      // Reset binaryChunk
      binaryChunk.clear();
    } else {
      std::runtime_error("Unknown chunk type: " + std::to_string(chunkType));
    }
  }
  file.close();
}
