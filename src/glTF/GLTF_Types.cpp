#include "GLTF_Types.hpp"
#include "JSON.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <memory>

using namespace gltf;

Accessor::Accessor(JSONnode jsonAccessor) {
    bufferView = findOptional<int>(jsonAccessor,"bufferView");
    byteOffset = findOptional<int>(jsonAccessor,"byteOffset");
    componentType = find<int>(jsonAccessor,"componentType");
    normalized = findOptional<bool>(jsonAccessor,"normalized");
    count = find<int>(jsonAccessor,"count");
    type = find<std::string>(jsonAccessor,"type");

    std::optional<JSONnode::nodeVector> maxNodes = findOptional<JSONnode::nodeVector>(jsonAccessor,"max").value();
    for(JSONnode m : maxNodes.value()) {
        if(std::holds_alternative<double>(m.value())) {
            max.push_back(std::get<double>(m.value()));
        } else {
            max.push_back(std::get<int>(m.value()));
        }
      }
    std::optional<JSONnode::nodeVector> minNodes = findOptional<JSONnode::nodeVector>(jsonAccessor,"min").value();
    for(JSONnode m : minNodes.value()) {
        if(std::holds_alternative<double>(m.value())) {
            min.push_back(std::get<double>(m.value()));
        } else {
            min.push_back(std::get<int>(m.value()));
        }
      }
    // Only 1 sparse node
    std::optional<JSONnode::nodeVector> jsonSparse = findOptional<JSONnode::nodeVector>(jsonAccessor,"sparse");
    if(jsonSparse) {
        sparse = Sparse(jsonSparse.value()[0]);
    }
  name = findOptional<std::string>(jsonAccessor,"name");
}

Accessor::Sparse::Sparse(JSONnode jsonSparse) {
    count = find<int>(jsonSparse,"count");
    std::vector<JSONnode> jsonSparseIndices = find<JSONnode::nodeVector>(jsonSparse,"indices");
    for(JSONnode jsonIndex : jsonSparseIndices) {
      indices.push_back(Index(jsonIndex));
    }
    std::vector<JSONnode> jsonSparseValues = find<JSONnode::nodeVector>(jsonSparse,"values");
    for(JSONnode jsonValue : jsonSparseValues) {
        values.push_back(Value(jsonValue));
  }
}

Accessor::Sparse::Sparse::Index::Index(JSONnode jsonIndex) {
      bufferView = find<int>(jsonIndex,"bufferView");
      byteOffset = findOptional<int>(jsonIndex,"byteOffset");
      componentType = find<int>(jsonIndex,"componentType");
}

Accessor::Sparse::Sparse::Value::Value(JSONnode jsonValue) {
      bufferView = find<int>(jsonValue,"bufferView");
      byteOffset = findOptional<int>(jsonValue,"byteOffset");
}

Buffer::Buffer(JSONnode jsonBuffer, std::vector<unsigned char> byteData) {
    uri = find<std::string>(jsonBuffer,"uri");
    byteLength = find<int>(jsonBuffer,"byteLength");
    // TODO
    // Next thing to fix:
    // can't wrap some objects (at least std::vector) in std::optional<> and access them without
    // declaring a new vector first
    // Can't check .value() at the end, because it fails with bad optional access if there's no value
    // mzero from haskell would make this trivial
    // best solution for now is probably to make all of these pointers and allocate them in the constructor
    // then, delete in the destructor if they are allocated
    // I could probably keep the optional wrapper around them as well if I'm declaring as new
    name = findOptional<std::string>(jsonBuffer, "name");
    data = byteData;
}

BufferView::BufferView(JSONnode jsonBufferView) {
    buffer = find<int>(jsonBufferView,"buffer");
    byteOffset = findOptional<int>(jsonBufferView,"byteOffset");
    byteLength = find<int>(jsonBufferView,"byteLength");
    byteStride = findOptional<int>(jsonBufferView,"byteStride");
    target = findOptional<int>(jsonBufferView,"target");
    name = findOptional<std::string>(jsonBufferView,"name");
}

Node::Node(JSONnode jsonNode) {
    camera = findOptional<int>(jsonNode, "camera");
    for(JSONnode child : findOptional<JSONnode::nodeVector>(jsonNode,"nodes").value_or(JSONnode::nodeVector())) {
    children.push_back(std::get<int>(child.value()));
      }
    skin = findOptional<int>(jsonNode, "skin");

    std::vector<double> matrixAcc;
    for(JSONnode val : findOptional<JSONnode::nodeVector>(jsonNode,"matrix").value_or(JSONnode::nodeVector())) {
        matrixAcc.push_back(std::get<double>(val.value()));
      }
    if(matrixAcc.size() == 16) matrix = glm::make_mat4(matrixAcc.data());

    mesh = findOptional<int>(jsonNode, "mesh");

    std::vector<double> rotationAcc;
    for(JSONnode val : findOptional<JSONnode::nodeVector>(jsonNode,"rotation").value_or(JSONnode::nodeVector())) {
        rotationAcc.push_back(std::get<double>(val.value()));
      }
    if(rotationAcc.size() == 4) rotation = glm::make_quat(rotationAcc.data());

    std::vector<double> scaleAcc;
    for(JSONnode val : findOptional<JSONnode::nodeVector>(jsonNode,"scale").value_or(JSONnode::nodeVector())) {
        scaleAcc.push_back(std::get<double>(val.value()));
      }
    if(scaleAcc.size() == 3) scale = glm::make_vec3(scaleAcc.data());

    std::vector<double> translationAcc;
    for(JSONnode val : findOptional<JSONnode::nodeVector>(jsonNode,"translation").value_or(JSONnode::nodeVector())) {
        translationAcc.push_back(std::get<double>(val.value()));
      }
    if(translationAcc.size() == 3) translation = glm::make_vec3(translationAcc.data());


    for(JSONnode weight : findOptional<JSONnode::nodeVector>(jsonNode,"weights").value_or(JSONnode::nodeVector())) {
        weights.push_back(std::get<double>(weight.value()));
      }

    name = findOptional<std::string>(jsonNode, "name");
}

Mesh::Mesh(JSONnode jsonMesh) {
    for(JSONnode jsonPrimitive : find<JSONnode::nodeVector>(jsonMesh,"primitives")) {
    primitives.push_back(Primitive(jsonPrimitive));
    }

    for(JSONnode weight : findOptional<JSONnode::nodeVector>(jsonMesh,"weights").value_or(JSONnode::nodeVector())) {
        weights.push_back(std::get<double>(weight.value()));
      }

    name = findOptional<std::string>(jsonMesh, "name");
}

Mesh::Mesh::Primitive::Primitive(JSONnode jsonPrimitive) {
   attributes = std::unique_ptr<Attributes>(new Attributes(find<JSONnode::nodeVector>(jsonPrimitive,"attributes")));
   indices = findOptional<int>(jsonPrimitive,"indices");
   material = findOptional<int>(jsonPrimitive,"material");
   mode = findOptional<int>(jsonPrimitive,"mode");
   // TODO targets
}

Mesh::Mesh::Primitive::Primitive::Attributes::Attributes(JSONnode::nodeVector jsonAttributes) {
    for(JSONnode attribute : jsonAttributes) {
        std::string key = attribute.key();
        // TODO finish filling this out
        if(key.compare("POSITION") == 0) {
            position = std::get<int>(attribute.value());
        } else if (key.compare("NORMAL") == 0) {
        } else if (key.compare("NORMAL") == 0) {
        } else if (key.compare("NORMAL") == 0) {
        } else if (key.compare("NORMAL") == 0) {
        } else if (key.compare("NORMAL") == 0) {
        } else if (key.compare("NORMAL") == 0) {
        }
    }
}

Scene::Scene(JSONnode jsonScene) {
        JSONnode::nodeVector jsonNodes = findOptional<JSONnode::nodeVector>(jsonScene,"nodes").value();
    for(JSONnode node : jsonNodes) {
        nodes.push_back(std::get<int>(node.value()));
    }
   name = findOptional<std::string>(jsonScene,"name");
}
