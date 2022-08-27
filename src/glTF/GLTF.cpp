#include "GLTF.hpp"

using namespace gltf;
GLTF::GLTF(std::string filePath) {
  std::ifstream file(filePath);
  JSONnode root = JSONnode(file);
  for (auto node : root.nodeList) {
    // https://opensource.com/article/21/2/ccc-method-pointers
    ((*this).*(nodeJumpTable.find(node.string)->second))(node);
  }
  file.close();
}

// TODO
// Redo this jump table, for loop, if statement, and class spam

void GLTF::accessorsLoader(JSONnode node) {
  for (auto accessorOBJ : node.nodeList) {
    Accessor accessor;
    for (auto key : accessorOBJ.nodeList) {
      if (key.string.compare("bufferView")) {
        accessor.bufferView = key.nodeList.front().nodeList.front().iValue;
      } else if (key.string.compare("byteOffset")) {
        accessor.byteOffset = key.nodeList.front().iValue;
      } else if (key.string.compare("componentType")) {
        accessor.componentType = key.nodeList.front().iValue;
      } else if (key.string.compare("normalized")) {
        accessor.normalized = key.nodeList.front().bValue;
      } else if (key.string.compare("count")) {
        accessor.count = key.nodeList.front().iValue;
      } else if (key.string.compare("type")) {
        accessor.type = key.nodeList.front().string;
      } else if (key.string.compare("max")) {
        for (auto num : key.nodeList) {
          accessor.max.push_back(num.nodeList.front().iValue);
        }
      } else if (key.string.compare("min")) {
        for (auto num : key.nodeList) {
          accessor.max.push_back(num.nodeList.front().iValue);
        }
      } else if (key.string.compare("sparse")) {
        for (auto k : key.nodeList) {
          if (k.string.compare("count")) {
            accessor.sparse.count = k.nodeList.front().iValue;
          } else if (k.string.compare("indices")) {
            Accessor::Sparse::Index index;
            for (auto i : k.nodeList) {
              if (i.string.compare("bufferView")) {
                index.bufferView = i.nodeList.front().iValue;
              } else if (i.string.compare("byteOffset")) {
                index.byteOffset = i.nodeList.front().iValue;
              } else if (i.string.compare("componentType")) {
                index.componentType = i.nodeList.front().iValue;
              }
            }
            accessor.sparse.indices.push_back(index);
          } else if (k.string.compare("values")) {
            Accessor::Sparse::Value value;
            for (auto val : k.nodeList) {
              if (val.string.compare("bufferView")) {
                value.bufferView = val.nodeList.front().iValue;
              } else if (val.string.compare("byteOffset")) {
                value.byteOffset = val.nodeList.front().iValue;
              }
            }
            accessor.sparse.values.push_back(value);
          }
        }
      } else if (key.string.compare("name")) {
        accessor.count = key.nodeList.front().iValue;
      }
    }
    _accessors.push_back(accessor);
  }
}
void GLTF::animationsLoader(JSONnode node) {}
void GLTF::assetLoader(JSONnode node) {}
void GLTF::buffersLoader(JSONnode node) {}
void GLTF::bufferViewsLoader(JSONnode node) {}
void GLTF::camerasLoader(JSONnode node) {}
void GLTF::extensionsUsedLoader(JSONnode node) {}
void GLTF::extensionsRequiredLoader(JSONnode node) {}
void GLTF::imagesLoader(JSONnode node) {}
void GLTF::materialsLoader(JSONnode node) {}
void GLTF::meshesLoader(JSONnode node) {}
void GLTF::nodesLoader(JSONnode node) {}
void GLTF::samplersLoader(JSONnode node) {}
void GLTF::sceneLoader(JSONnode node) {}
void GLTF::scenesLoader(JSONnode node) {}
void GLTF::skinsLoader(JSONnode node) {}
void GLTF::texturesLoader(JSONnode node) {}

/*

  {"Accessor", {{"bufferView", int}, {"byteOffset", int}, {"componentType", int}, {"normalized", bool}},
  }





 */
