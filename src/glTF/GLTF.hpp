// DO NOT USE
// USE GLB FILES FOR NOW
#ifndef GLTF_H_
#define GLTF_H_

#include "JSON.hpp"
#include <glm/glm.hpp>
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "GLTF_Types.hpp"
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <unordered_map>
#include <vector>

// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#properties-reference

namespace gltf {
class GLTF {
  public:
    GLTF(std::string filePath);
    ~GLTF();

    // TODO
    // Make private
    // Add const getters
    std::vector<Accessor> accessors;
    std::vector<Buffer> buffers;
    std::vector<BufferView> bufferViews;
    int scene;
    std::vector<Scene> scenes;
    std::vector<Node> nodes;
    std::vector<Mesh> meshes;
    Asset* asset;

  private:
    std::string getFileExtension(std::string filePath);
    std::vector<unsigned char> loadURI(std::string uri, int byteLength);
    void loadGLTF(std::string filePath);
    uint32_t readuint32(std::ifstream& file);
    void loadGLB(std::string filePath);
    JSONnode* jsonRoot = nullptr;
};
}; // namespace gltf

#endif // GLTF_H_
