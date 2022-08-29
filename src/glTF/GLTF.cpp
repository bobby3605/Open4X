#include "GLTF.hpp"
#include "base64.hpp"

std::string GLTF::getFileExtension(std::string filePath) { return filePath.substr(filePath.find_last_of(".")); }

void GLTF::loadURI(std::string uri, int byteLength) {
  // Check what type of buffer

  if (uri.find("data:application/octet-stream;base64,") != std::string::npos) {
    // base64 header is 37 characters long
    std::vector<unsigned char> byteData = base64ToUChar(uri.substr(37));
  }
}

void GLTF::loadGLTF(std::string filePath) {
  std::ifstream file(filePath);
  // Remove extra data from the start of the file
  while (file.peek() != '{') {
    file.seekg(1, std::ios::cur);
  }
  // Get JSON of GLTF
  jsonRoot = new JSONnode(file);
  // Get data from GLTF JSON
  // For some reason, I need this extra buffers variable, otherwise I crash with basic_string::_M_create
  std::vector<JSONnode> buffers = std::get<4>(jsonRoot->find("buffers").value());
  for (JSONnode buffer : buffers) {
    // uri isn't required, but when would you have a buffer without a uri?
    // https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#reference-buffer
    loadURI(std::get<std::string>(buffer.find("uri").value()), std::get<int>(buffer.find("byteLength").value()));
  }
  file.close();
}

uint32_t GLTF::readuint32(std::ifstream &file) {
  uint32_t buffer;
  file.read((char *)&buffer, sizeof(uint32_t));
  return buffer;
}

void GLTF::loadGLB(std::string filePath) {
  std::ifstream file(filePath.c_str(), std::ios::binary);
  // Get header
  uint32_t magic = readuint32(file);
  uint32_t version = readuint32(file);
  uint32_t length = readuint32(file);

  while (file.tellg() < length) {
    // Get chunk header
    uint32_t chunkLength = readuint32(file);
    uint32_t chunkType = readuint32(file);

    unsigned char dataBuffer;
    if (chunkType == 0x4E4F534A) {
      if (jsonRoot) {
        throw std::runtime_error("Found second JSON chunk in glb");
      }
      // JSON chunk
      std::stringstream jsonString;
      for (uint32_t i = 0; i < chunkLength; i += sizeof(dataBuffer)) {
        file.read((char *)&dataBuffer, sizeof(dataBuffer));
        jsonString << dataBuffer;
      }
      // Remove any extra data at the start of the file
      while (jsonString.peek() != '{') {
        jsonString.seekp(1, std::ios::cur);
      }
      jsonRoot = new JSONnode(jsonString);
    } else if (chunkType == 0x004E4942) {
      // Binary chunk
      std::vector<unsigned char> binaryChunk;
      for (uint32_t i = 0; i < chunkLength; i += sizeof(dataBuffer)) {
        file.read((char *)&dataBuffer, sizeof(dataBuffer));
        binaryChunk.push_back(dataBuffer);
      }
      buffers.push_back(binaryChunk);
      // Reset binaryChunk
      binaryChunk.clear();
    } else {
      throw std::runtime_error("Unknown chunk type: " + std::to_string(chunkType));
    }
  }
  file.close();
}

GLTF::~GLTF() { delete jsonRoot; }

GLTF::GLTF(std::string filePath) {
  if (getFileExtension(filePath).compare(".gltf") == 0) {
    loadGLTF(filePath);
  } else if (getFileExtension(filePath).compare(".glb") == 0) {
    loadGLB(filePath);
  } else {
    std::cout << "Unsupported file: " << filePath << std::endl;
  }
}
