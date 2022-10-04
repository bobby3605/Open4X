#include "GLTF.hpp"
#include "GLTF_Types.hpp"
#include "JSON.hpp"
#include "base64.hpp"
#include <fstream>

using namespace gltf;


std::string GLTF::getFileExtension(std::string filePath) { return filePath.substr(filePath.find_last_of(".")); }

std::vector<unsigned char> GLTF::loadURI(std::string uri, int byteLength) {
  // Check what type of buffer
  if (uri.find("data:application/octet-stream;base64,") != std::string::npos) {
    // base64 header is 37 characters long
    return base64ToUChar(uri.substr(37));
  } else if (getFileExtension(uri).compare(".bin") == 0) {
    std::ifstream file(uri, std::ios::binary);
    unsigned char dataBuffer;
    std::vector<unsigned char> data;
    while(file.tellg() < byteLength){
        file.read((char *)&dataBuffer, sizeof(dataBuffer));
        data.push_back(dataBuffer);
    }
    return data;
  }
  throw std::runtime_error("Failed to parse uri: " + uri);
}

void GLTF::loadGLTF(std::string filePath) {
  std::ifstream file(filePath);
  // Remove extra data from the start of the file
  while (file.peek() != '{') {
    file.seekg(1, std::ios::cur);
  }
  // Get JSON of GLTF
  jsonRoot = new JSONnode(file);

  // TODO put this into a separate function so loadGLB can call it
  // Get data from GLTF JSON
  // For some reason, I need this extra variable, otherwise I crash with basic_string::_M_create
  JSONnode::nodeVector jsonBuffers = find<JSONnode::nodeVector>(jsonRoot, "buffers");
  for (JSONnode jsonBuffer : jsonBuffers) {
    buffers.push_back(Buffer(jsonBuffer,loadURI(find<std::string>(jsonBuffer, "uri"), find<int>(jsonBuffer, "byteLength"))));
  }

  JSONnode::nodeVector jsonBufferViews = find<JSONnode::nodeVector>(jsonRoot, "bufferViews");
  for (JSONnode jsonBufferView : jsonBufferViews) {
    bufferViews.push_back(BufferView(jsonBufferView));
  }

  JSONnode::nodeVector jsonAccessors = find<JSONnode::nodeVector>(jsonRoot,"accessors");
  for(JSONnode jsonAccessor : jsonAccessors) {
    accessors.push_back(Accessor(jsonAccessor));
    }

  std::optional<int> jsonDefaultScene = findOptional<int>(jsonRoot,"scene");
  if(jsonDefaultScene) scene = jsonDefaultScene.value();

  std::optional<JSONnode::nodeVector> jsonScenes = findOptional<JSONnode::nodeVector>(jsonRoot, "scenes");
  if(jsonScenes) {
    for(JSONnode jsonScene : jsonScenes.value()) {
      scenes.push_back(Scene(jsonScene));
    }
  }

  std::optional<JSONnode::nodeVector> jsonNodes = findOptional<JSONnode::nodeVector>(jsonRoot, "nodes");
  if(jsonNodes) {
    for(JSONnode jsonNode : jsonNodes.value()) {
      nodes.push_back(Node(jsonNode));
    }
  }

  std::optional<JSONnode::nodeVector> jsonMeshes = findOptional<JSONnode::nodeVector>(jsonRoot, "meshes");
  if(jsonMeshes) {
    for(JSONnode jsonMesh : jsonMeshes.value()) {
      meshes.push_back(Mesh(jsonMesh));
    }
  }

  file.close();
}

uint32_t GLTF::readuint32(std::ifstream &file) {
  uint32_t buffer;
  file.read((char *)&buffer, sizeof(uint32_t));
  return buffer;
}

void GLTF::loadGLB(std::string filePath) {
  std::ifstream file(filePath, std::ios::binary);
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
      //buffers.push_back(binaryChunk);
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
