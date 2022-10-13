#include "vulkan_node.hpp"
#include "rapidjson_model.hpp"
#include <memory>

VulkanNode::VulkanNode(RapidJSON_Model* model, int nodeID, std::shared_ptr<std::map<int, std::shared_ptr<Mesh>>> meshIDMap)
    : model{model}, nodeID{nodeID}, meshIDMap{meshIDMap} {
    if (model->nodes[nodeID].mesh.has_value()) {
        meshID = model->nodes[nodeID].mesh.value();
        if (meshIDMap->count(meshID.value()) == 0) {
            // gl_BaseInstance cannot be nodeID, since only nodes with a mesh value are rendered
            meshIDMap->insert({meshID.value(), std::make_shared<Mesh>(model, model->nodes[nodeID].mesh.value(), meshIDMap->size())});
        } else {
            for (std::shared_ptr<Mesh::Primitive> primitive : meshIDMap->find(meshID.value())->second->primitives) {
                // Requires an index buffer into the SSBO model matrix buffer
                // gl_DrawID will increase per primitive and gl_BaseInstance cannot be used as a model matrix index,
                // since that cannot be used for instanced rendering
                // The alternative is to duplicate a node's model matrix per primitive per instance
                // TODO
                // Implement a better solution than an index buffer
                ++primitive->indirectDraw.instanceCount;
            }
        }
    }
}

glm::mat4 const VulkanNode::modelMatrix() { return _modelMatrix; }

Mesh::Mesh(RapidJSON_Model* model, int meshID, int gl_BaseInstance) {
    for (RapidJSON_Model::Mesh::Primitive primitive : model->meshes[meshID].primitives) {
        primitives.push_back(std::make_shared<Mesh::Primitive>(model, meshID, primitive, gl_BaseInstance));
    }
}

Mesh::Primitive::Primitive(RapidJSON_Model* model, int meshID, RapidJSON_Model::Mesh::Primitive primitive, int gl_BaseInstance) {
    RapidJSON_Model::Accessor* accessor;

    if (primitive.attributes->position.has_value()) {
        accessor = &model->accessors[primitive.attributes->position.value()];
        // Set of sparse indices
        std::set<unsigned short> sparseIndices;
        // Vector of vertices to replace with
        std::vector<glm::vec3> sparseValues;
        if (accessor->sparse.has_value()) {
            RapidJSON_Model::BufferView* bufferView;
            for (uint32_t count_index = 0; count_index < accessor->sparse->count; ++count_index) {
                // load the index
                bufferView = &model->bufferViews[accessor->sparse->indices->bufferView];
                int offset = accessor->sparse->indices->byteOffset + bufferView->byteOffset +
                             count_index * (bufferView->byteStride.has_value() ? bufferView->byteStride.value() : sizeof(unsigned short));
                sparseIndices.insert(*(reinterpret_cast<unsigned short*>(model->buffers[bufferView->buffer].data.data() + offset)));
                // load the vertex
                bufferView = &model->bufferViews[accessor->sparse->values->bufferView];
                offset = accessor->sparse->values->byteOffset + bufferView->byteOffset +
                         count_index * (bufferView->byteStride.has_value() ? bufferView->byteStride.value() : sizeof(glm::vec3));
                sparseValues.push_back(*(reinterpret_cast<glm::vec3*>(model->buffers[bufferView->buffer].data.data() + offset)));
            }
        }
        std::vector<glm::vec3>::iterator sparseValuesIterator = sparseValues.begin();

        for (uint32_t count_index = 0; count_index < accessor->count; ++count_index) {
            Vertex vertex{};
            if (sparseIndices.count(count_index) == 1) {
                vertex.pos = *sparseValuesIterator;
                ++sparseValuesIterator;
            } else {
                vertex.pos = loadAccessor<glm::vec3>(model, accessor, count_index);
            }
            vertex.texCoord = {0.0, 0.0};
            vertex.color = {1.0f, 1.0f, 1.0f};
            vertices.push_back(vertex);
        }
    }
    if (primitive.indices.has_value()) {
        accessor = &model->accessors[primitive.indices.value()];
        for (uint32_t count_index = 0; count_index < accessor->count; ++count_index) {
            indices.push_back(loadAccessor<unsigned short>(model, accessor, count_index));
        }
    }
    // TODO
    // Generate index buffer if it does not exist
    VkDrawIndexedIndirectCommand indirectDraw{};
    indirectDraw.indexCount = indices.size();
    indirectDraw.instanceCount = 1;
    indirectDraw.firstIndex = 0;
    indirectDraw.vertexOffset = 0;
    indirectDraw.firstInstance = gl_BaseInstance;
}
