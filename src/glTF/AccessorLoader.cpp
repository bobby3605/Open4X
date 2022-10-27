#include "AccessorLoader.hpp"

template <typename OT> AccessorLoader<OT>::AccessorLoader(GLTF* model, GLTF::Accessor* accessor) : _accessor{accessor} {
    _bufferView = &model->bufferViews[accessor->bufferView.value()];
    uint32_t baseOffset = accessor->byteOffset + _bufferView->byteOffset;
    stride = _bufferView->byteStride.has_value() ? _bufferView->byteStride.value() : sizeSwitch(accessor->componentType, accessor->type);
    data = model->buffers[_bufferView->buffer].data.data();
    data += baseOffset;
    _componentType = accessor->componentType;
    getDataF = getComponent<OT>();
}

template <typename OT>
AccessorLoader<OT>::AccessorLoader(GLTF* model, GLTF::Accessor* accessor, GLTF::BufferView* bufferView, uint32_t accessorByteOffset,
                                   uint32_t componentType, std::string type)
    : _accessor{accessor} {
    _bufferView = bufferView;
    uint32_t baseOffset = accessorByteOffset + _bufferView->byteOffset;
    stride = _bufferView->byteStride.has_value() ? _bufferView->byteStride.value() : sizeSwitch(componentType, type);
    _componentType = componentType;
    data = model->buffers[_bufferView->buffer].data.data();
    data += baseOffset;
    getDataF = getComponent<OT>();
}

template <typename OT> OT AccessorLoader<OT>::at(uint32_t count_index) { return getDataF(data, count_index * stride); }
