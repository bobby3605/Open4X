#include "rapidjson_model.hpp"
#include "../../external/rapidjson/istreamwrapper.h"
#include "../glTF/base64.hpp"
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <iostream>

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

    if (d.HasMember("animations")) {
        Value& animationJSON = d["animations"];
        assert(animationJSON.IsArray());
        for (SizeType i = 0; i < animationJSON.Size(); ++i) {
            animations.push_back(animationJSON[i]);
        }
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
    if (nodeJSON.HasMember("mesh")) {
        Value& meshJSON = nodeJSON["mesh"];
        if (meshJSON.IsInt()) {
            mesh = meshJSON.GetInt();
        }
    }
    if (nodeJSON.HasMember("children")) {
        Value& childrenJSON = nodeJSON["children"];
        assert(childrenJSON.IsArray());
        for (SizeType i = 0; i < childrenJSON.Size(); ++i) {
            children.push_back(childrenJSON[i].GetInt());
        }
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

    if (nodeJSON.HasMember("matrix")) {
        Value& matrixJSON = nodeJSON["matrix"];
        if (matrixJSON.IsArray()) {
            assert(matrixJSON.Size() == 16);
            std::vector<float> matrixJSONBuffer(16);
            for (SizeType i = 0; i < matrixJSON.Size(); ++i) {
                matrixJSONBuffer.push_back(matrixJSON[i].GetFloat());
            }
            // TODO
            // Might need glm::transpose
            // need to test it
            matrix = glm::make_mat4(matrixJSONBuffer.data());
        }
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
            } else if (uri->substr(uri->find_last_of(".")).compare(".bin") == 0) {
                std::ifstream file("assets/glTF/" + uri.value(), std::ios::binary);
                if (!file.is_open()) {
                    throw std::runtime_error("failed to open file: " + uri.value());
                } else {
                    // https://stackoverflow.com/a/21802936
                    file.unsetf(std::ios::skipws);
                    // get its size:
                    std::streampos fileSize;
                    file.seekg(0, std::ios::end);
                    fileSize = file.tellg();
                    file.seekg(0, std::ios::beg);
                    // reserve capacity
                    data.reserve(fileSize);
                    // read the data:
                    data.insert(data.begin(), std::istream_iterator<unsigned char>(file), std::istream_iterator<unsigned char>());
                }
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
    if (accessorJSON.HasMember("sparse")) {
        Value& sparseJSON = accessorJSON["sparse"];
        sparse = Sparse(sparseJSON);
    }
}

RapidJSON_Model::Accessor::Sparse::Sparse(Value& sparseJSON) {
    assert(sparseJSON.IsObject());
    Value& countJSON = sparseJSON["count"];
    assert(countJSON.IsInt());
    count = countJSON.GetInt();
    Value& indicesJSON = sparseJSON["indices"];
    assert(indicesJSON.IsObject());
    indices = std::make_shared<RapidJSON_Model::Accessor::Sparse::Index>(indicesJSON);
    Value& valuesJSON = sparseJSON["values"];
    values = std::make_shared<RapidJSON_Model::Accessor::Sparse::SparseValue>(valuesJSON);
}

RapidJSON_Model::Accessor::Sparse::Sparse::Index::Index(Value& indexJSON) {
    assert(indexJSON.IsObject());
    Value& bufferViewJSON = indexJSON["bufferView"];
    assert(bufferViewJSON.IsInt());
    bufferView = bufferViewJSON.GetInt();
    if (indexJSON.HasMember("byteOffset")) {
        Value& byteOffsetJSON = indexJSON["byteOffset"];
        assert(byteOffsetJSON.IsInt());
        byteOffset = byteOffsetJSON.GetInt();
    } else {
        byteOffset = 0;
    }
    Value& componentTypeJSON = indexJSON["componentType"];
    assert(componentTypeJSON.IsInt());
    componentType = componentTypeJSON.GetInt();
}

RapidJSON_Model::Accessor::Sparse::SparseValue::SparseValue(Value& valueJSON) {
    assert(valueJSON.IsObject());
    Value& bufferViewJSON = valueJSON["bufferView"];
    assert(bufferViewJSON.IsInt());
    bufferView = bufferViewJSON.GetInt();
    if (valueJSON.HasMember("byteOffset")) {
        Value& byteOffsetJSON = valueJSON["byteOffset"];
        assert(byteOffsetJSON.IsInt());
        byteOffset = byteOffsetJSON.GetInt();
    } else {
        byteOffset = 0;
    }
}

RapidJSON_Model::Animation::Animation(Value& animationJSON) {
    assert(animationJSON.IsObject());
    Value& samplersJSON = animationJSON["samplers"];
    assert(samplersJSON.IsArray());
    for (SizeType i = 0; i < samplersJSON.Size(); ++i) {
        samplers.push_back(std::make_shared<Animation::Sampler>(samplersJSON[i]));
    }
    Value& channelsJSON = animationJSON["channels"];
    assert(channelsJSON.IsArray());
    for (SizeType i = 0; i < channelsJSON.Size(); ++i) {
        channels.push_back(std::make_shared<Animation::Channel>(channelsJSON[i]));
    }
}

RapidJSON_Model::Animation::Sampler::Sampler(Value& samplerJSON) {
    assert(samplerJSON.IsObject());
    Value& inputJSON = samplerJSON["input"];
    assert(inputJSON.IsInt());
    inputIndex = inputJSON.GetInt();
    Value& interpolationJSON = samplerJSON["interpolation"];
    assert(interpolationJSON.IsString());
    interpolation = interpolationJSON.GetString();
    Value& outputJSON = samplerJSON["output"];
    assert(outputJSON.IsInt());
    outputIndex = outputJSON.GetInt();
}

RapidJSON_Model::Animation::Channel::Channel(Value& channelJSON) {
    assert(channelJSON.IsObject());
    Value& samplerJSON = channelJSON["sampler"];
    assert(samplerJSON.IsInt());
    sampler = samplerJSON.GetInt();
    Value& targetJSON = channelJSON["target"];
    assert(targetJSON.IsObject());
    target = std::make_shared<Target>(targetJSON);
}

RapidJSON_Model::Animation::Channel::Target::Target(Value& targetJSON) {
    assert(targetJSON.IsObject());
    Value& nodeJSON = targetJSON["node"];
    assert(nodeJSON.IsInt());
    node = nodeJSON.GetInt();
    Value& pathJSON = targetJSON["path"];
    assert(pathJSON.IsString());
    path = pathJSON.GetString();
}
