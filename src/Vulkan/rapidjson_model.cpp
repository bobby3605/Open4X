#include "rapidjson_model.hpp"
#include "../../external/rapidjson/istreamwrapper.h"
#include "../glTF/base64.hpp"
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>

RapidJSON_Model::RapidJSON_Model(std::string filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filePath);
    }

    IStreamWrapper fileStream(file);

    d.ParseStream(fileStream);

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

    Value& accessorJSON = d["accessors"];
    assert(accessorJSON.IsArray());
    for (SizeType i = 0; i < accessorJSON.Size(); ++i) {
        accessors.push_back(accessorJSON[i]);
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
    if (nodeJSON.HasMember("translation")) {
        Value& translationJSON = nodeJSON["translation"];
        if (translationJSON.IsArray()) {
            assert(translationJSON.Size() == 3);
            std::vector<float> translationJSONBuffer;
            for (SizeType i = 0; i < translationJSON.Size(); ++i) {
                translationJSONBuffer.push_back(translationJSON[i].GetFloat());
            }
            translation = glm::make_vec3(translationJSONBuffer.data());
        }
    }
    if (nodeJSON.HasMember("rotation")) {
        Value& rotationJSON = nodeJSON["rotation"];
        if (rotationJSON.IsArray()) {
            assert(rotationJSON.Size() == 4);
            std::vector<float> rotationJSONBuffer;
            for (SizeType i = 0; i < rotationJSON.Size(); ++i) {
                rotationJSONBuffer.push_back(rotationJSON[i].GetFloat());
            }
            rotation = glm::make_quat(rotationJSONBuffer.data());
        }
    }
    if (nodeJSON.HasMember("scale")) {
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
    matrix = glm::translate(glm::mat4(1.0f), translation) * glm::toMat4(rotation) * glm::scale(scale);
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
    assert(primitiveJSON.IsObject());
    Value& attributesJSON = primitiveJSON["attributes"];
    assert(attributesJSON.IsObject());
    attributes = std::make_shared<Attributes>(attributesJSON);
    if (primitiveJSON.HasMember("indices")) {
        Value& indicesJSON = primitiveJSON["indices"];
        if (indicesJSON.IsInt()) {
            indices = indicesJSON.GetInt();
        }
    }
}

RapidJSON_Model::Mesh::Primitive::Attributes::Attributes(Value& attributesJSON) {
    if (attributesJSON.HasMember("POSITION")) {
        Value& positionJSON = attributesJSON["POSITION"];
        if (positionJSON.IsInt()) {
            position = positionJSON.GetInt();
        }
    }
    if (attributesJSON.HasMember("NORMAL")) {
        Value& normalJSON = attributesJSON["NORMAL"];
        if (normalJSON.IsInt()) {
            normal = normalJSON.GetInt();
        }
    }
}

RapidJSON_Model::Buffer::Buffer(Value& bufferJSON) {
    assert(bufferJSON.IsObject());
    if (bufferJSON.HasMember("uri")) {
        Value& uriJSON = bufferJSON["uri"];
        if (uriJSON.IsString()) {
            uri = uriJSON.GetString();
            std::string::size_type pos;
            if ((pos = uri->find("base64,")) != std::string::npos) {
                data = base64ToUChar(uri->substr(pos + 7));
            }
        }
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
    if (bufferViewJSON.HasMember("byteOffset")) {
        Value& byteOffsetJSON = bufferViewJSON["byteOffset"];
        if (byteOffsetJSON.IsInt()) {
            byteOffset = byteOffsetJSON.GetInt();
        }
    }
    Value& byteLengthJSON = bufferViewJSON["byteLength"];
    assert(byteLengthJSON.IsInt());
    byteLength = byteLengthJSON.GetInt();
    if (bufferViewJSON.HasMember("byteStride")) {
        Value& byteStrideJSON = bufferViewJSON["byteStride"];
        if (byteStrideJSON.IsInt()) {
            byteStride = byteStrideJSON.GetInt();
        }
    }
    if (bufferViewJSON.HasMember("target")) {
        Value& targetJSON = bufferViewJSON["target"];
        if (targetJSON.IsInt()) {
            target = targetJSON.GetInt();
        }
    }
}

RapidJSON_Model::Accessor::Accessor(Value& accessorJSON) {
    assert(accessorJSON.IsObject());
    if (accessorJSON.HasMember("bufferView")) {
        Value& bufferViewJSON = accessorJSON["bufferView"];
        if (bufferViewJSON.IsInt()) {
            bufferView = bufferViewJSON.GetInt();
        }
    }
    if (accessorJSON.HasMember("byteOffset")) {
        Value& byteOffsetJSON = accessorJSON["byteOffset"];
        if (byteOffsetJSON.IsInt()) {
            byteOffset = byteOffsetJSON.GetInt();
        }
    }
    Value& componentTypeJSON = accessorJSON["componentType"];
    assert(componentTypeJSON.IsInt());
    componentType = componentTypeJSON.GetInt();
    Value& countJSON = accessorJSON["count"];
    assert(countJSON.IsInt());
    count = countJSON.GetInt();
    Value& typeJSON = accessorJSON["type"];
    assert(typeJSON.IsString());
    type = typeJSON.GetString();
    if (accessorJSON.HasMember("max")) {
        Value& maxJSON = accessorJSON["max"];
        if (maxJSON.IsArray()) {
            for (Value::ValueIterator itr = maxJSON.Begin(); itr != maxJSON.End(); ++itr) {
                max.push_back(itr->GetFloat());
            }
        }
    }
    if (accessorJSON.HasMember("min")) {
        Value& minJSON = accessorJSON["min"];
        if (minJSON.IsArray()) {
            for (Value::ValueIterator itr = minJSON.Begin(); itr != minJSON.End(); ++itr) {
                min.push_back(itr->GetFloat());
            }
        }
    }
}
