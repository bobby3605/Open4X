#include "GLTF_Types.hpp"
#include "JSON.hpp"

using namespace gltf;

Accessor::Accessor(JSONnode jsonAccessor) {
    bufferView = findOptional<int>(jsonAccessor,"bufferView");
    byteOffset = findOptional<int>(jsonAccessor,"byteOffset");
    componentType = find<int>(jsonAccessor,"componentType");
    normalized = findOptional<bool>(jsonAccessor,"normalized");
    count = find<int>(jsonAccessor,"count");
    type = find<std::string>(jsonAccessor,"type");
    for(JSONnode m : findOptional<JSONnode::nodeVector>(jsonAccessor,"max").value()) {
    max.value().push_back(std::get<double>(m.value()));
      }
    for(JSONnode m : findOptional<JSONnode::nodeVector>(jsonAccessor,"min").value()) {
    min.value().push_back(std::get<double>(m.value()));
      }
    // Only 1 sparse node
    std::optional<JSONnode> jsonSparse = findOptional<JSONnode::nodeVector>(jsonAccessor,"sparse").value()[0];
    if(jsonSparse) {
        sparse = Sparse(jsonSparse.value());
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

Buffer::Buffer(JSONnode jsonBuffer, std::vector<unsigned char> data) {
    uri = find<std::string>(jsonBuffer,"uri");
    byteLength = find<int>(jsonBuffer,"byteLength");
    name = findOptional<std::string>(jsonBuffer, "name");
    data = data;
}

BufferView::BufferView(JSONnode jsonBufferView) {
    buffer = find<int>(jsonBufferView,"buffer");
    byteOffset = findOptional<int>(jsonBufferView,"byteOffset");
    byteLength = find<int>(jsonBufferView,"byteLength");
    byteStride = findOptional<int>(jsonBufferView,"byteStride");
    target = findOptional<int>(jsonBufferView,"target");
    name = findOptional<std::string>(jsonBufferView,"name");
}

Scene::Scene(JSONnode jsonScene) {
    for(JSONnode node : findOptional<JSONnode::nodeVector>(jsonScene,"nodes").value()) {
        nodes.value().push_back(std::get<int>(node.value()));
    }
   name = findOptional<std::string>(jsonScene,"name");
}
