#include "AccessorLoader.hpp"

template <typename OT> AccessorLoader<OT>::AccessorLoader(GLTF* model, GLTF::Accessor* accessor) : _accessor{accessor} {
    _bufferView = &model->bufferViews[accessor->bufferView.value()];
    baseOffset = accessor->byteOffset + _bufferView->byteOffset;
    multiplyOffset =
        _bufferView->byteStride.has_value() ? _bufferView->byteStride.value() : sizeSwitch(accessor->componentType, accessor->type);
    data = model->buffers[_bufferView->buffer].data.data();
    _componentType = accessor->componentType;
}

template <typename OT>
AccessorLoader<OT>::AccessorLoader(GLTF* model, GLTF::Accessor* accessor, GLTF::BufferView* bufferView, uint32_t accessorByteOffset,
                                   uint32_t componentType, std::string type)
    : _accessor{accessor} {
    _bufferView = bufferView;
    baseOffset = accessorByteOffset + _bufferView->byteOffset;
    multiplyOffset = _bufferView->byteStride.has_value() ? _bufferView->byteStride.value() : sizeSwitch(componentType, type);
    _componentType = componentType;
    data = model->buffers[_bufferView->buffer].data.data();
}

template <typename OT> OT AccessorLoader<OT>::at(uint32_t count_index) {
    uint32_t offset = baseOffset + count_index * multiplyOffset;
    return getComponent<OT>(offset);
}
