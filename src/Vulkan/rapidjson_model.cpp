#include "rapidjson_model.hpp"
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

RapidJSON_Model::RapidJSON_Model(std::string filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filePath);
    }

    d.ParseStream(file);

    Value& scenesJSON = d["scenes"];
    assert(scenesJSON.IsArray());
    for (SizeType i = 0; i < scenesJSON.Size(); ++i) {
        scenes.push_back(scenesJSON[i]);
    }

    Value& nodesJSON = d["nodes"];
    assert(nodesJSON.IsArray());
    for (SizeType i = 0; i < nodesJSON.Size(); ++i) {
        nodes.push_back(nodesJSON[i]);
    }

    Value& meshesJSON = d["meshes"];
    assert(meshesJSON.IsArray());
    for (SizeType i = 0; i < meshesJSON.Size(); ++i) {
        meshes.push_back(meshesJSON[i]);
    }

    Value& buffersJSON = d["buffers"];
    assert(buffersJSON.IsArray());
    for (SizeType i = 0; i < buffersJSON.Size(); ++i) {
        buffers.push_back(buffersJSON[i]);
    }

    Value& bufferViewJSON = d["bufferViews"];
    assert(bufferViewJSON.IsArray());
    for (SizeType i = 0; i < bufferViewJSON.Size(); ++i) {
        bufferViews.push_back(bufferViewJSON[i]);
    }
}

RapidJSON_Model::Scene::Scene(Value& sceneJSON) {
    assert(sceneJSON.IsObject());
    Value& nodesJSON = sceneJSON["nodes"];
    assert(nodesJSON.IsArray());
    for (SizeType i = 0; i < nodesJSON.Size(); ++i) {
        nodes.push_back(nodesJSON[i].GetInt());
    }
}

RapidJSON_Model::Node::Node(Value& nodeJSON) {
    assert(nodeJSON.IsObject());
    Value& meshJSON = nodeJSON["mesh"];
    if (meshJSON.IsInt()) {
        mesh = meshJSON.GetInt();
    }
    Value& translationJSON = nodeJSON["translation"];
    if (translationJSON.IsArray()) {
        assert(translationJSON.Size() == 3);
        std::vector<float> translationJSONBuffer;
        for (SizeType i = 0; i < translationJSON.Size(); ++i) {
            translationJSONBuffer.push_back(translationJSON[i].GetFloat());
        }
        translation = glm::make_vec3(translationJSONBuffer.data());
    }
    Value& rotationJSON = nodeJSON["rotation"];
    if (rotationJSON.IsArray()) {
        assert(rotationJSON.Size() == 4);
        std::vector<float> rotationJSONBuffer;
        for (SizeType i = 0; i < rotationJSON.Size(); ++i) {
            rotationJSONBuffer.push_back(rotationJSON[i].GetFloat());
        }
        rotation = glm::make_quat(rotationJSONBuffer.data());
    }
    Value& scaleJSON = nodeJSON["scale"];
    if (scaleJSON.IsArray()) {
        assert(scaleJSON.Size() == 3);
        std::vector<float> scaleJSONBuffer;
        for (SizeType i = 0; i < scaleJSON.Size(); ++i) {
            scaleJSONBuffer.push_back(scaleJSON[i].GetFloat());
        }
        scale = glm::make_vec3(scaleJSONBuffer.data());
    }
}

RapidJSON_Model::Mesh::Mesh(Value& meshJSON) {
    assert(meshJSON.IsObject());
    Value& primitivesJSON = meshJSON["primitives"];
    assert(primitivesJSON.IsArray());
    for (SizeType i = 0; i < primitivesJSON.Size(); ++i) {
        primitives.push_back(primitivesJSON[i]);
    }
}

RapidJSON_Model::Mesh::Primitive::Primitive(Value& primitiveJSON) {
    assert(primitiveJSON.IsArray());
    Value& attributesJSON = primitiveJSON["attributes"];
    assert(attributesJSON.IsObject());
    attributes = std::make_shared<Attributes>(attributesJSON);
    Value& indicesJSON = primitiveJSON["indices"];
    if (indicesJSON.IsInt()) {
        indices = indicesJSON.GetInt();
    }
}

RapidJSON_Model::Mesh::Primitive::Attributes::Attributes(
    Value& attributesJSON) {
    Value& positionJSON = attributesJSON["POSITION"];
    if (positionJSON.IsInt()) {
        position = positionJSON.GetInt();
    }
    Value& normalJSON = attributesJSON["NORMAL"];
    if (normalJSON.IsInt()) {
        normal = normalJSON.GetInt();
    }
}

RapidJSON_Model::Buffer::Buffer(Value& bufferJSON) {
    assert(bufferJSON.IsObject());
    Value& uriJSON = bufferJSON["uri"];
    if (uriJSON.IsString()) {
        uri = uriJSON.GetString();
    }
    Value& byteLengthJSON = bufferJSON["byteLength"];
    assert(byteLengthJSON.IsInt());
    byteLength = byteLengthJSON.GetInt();
}

RapidJSON_Model::BufferView::BufferView(Value& bufferViewJSON) {
    assert(bufferViewJSON.IsObject());
    Value& bufferJSON = bufferViewJSON["buffer"];
    assert(bufferJSON.IsInt());
    buffer = bufferJSON.GetInt();
    Value& byteOffsetJSON = bufferViewJSON["byteOffset"];
    if (byteOffsetJSON.IsInt()) {
        byteOffset = byteOffsetJSON.GetInt();
    }
    Value& byteLengthJSON = bufferViewJSON["byteLength"];
    assert(byteLengthJSON.IsInt());
    byteLength = byteLengthJSON.GetInt();
    Value& byteStrideJSON = bufferViewJSON["byteStride"];
    if (byteStrideJSON.IsInt()) {
        byteStride = byteStrideJSON.GetInt();
    }
    Value& targetJSON = bufferViewJSON["target"];
    if (targetJSON.IsInt()) {
        target = targetJSON.GetInt();
    }
}
