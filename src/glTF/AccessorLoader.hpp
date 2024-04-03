#ifndef ACCESSORLOADER_H_
#define ACCESSORLOADER_H_
#include "GLTF.hpp"
#include <cstdint>
#include <glm/glm.hpp>
#include <string>

template <typename OT> class AccessorLoader {
  public:
    AccessorLoader(GLTF* model, GLTF::Accessor* accessor) : _accessor{accessor} {
        _bufferView = &model->bufferViews[accessor->bufferView.value()];
        uint32_t baseOffset = accessor->byteOffset + _bufferView->byteOffset;
        stride =
            _bufferView->byteStride.has_value() ? _bufferView->byteStride.value() : sizeSwitch(accessor->componentType, accessor->type);
        data = model->buffers[_bufferView->buffer].data.data();
        data += baseOffset;
        _componentType = accessor->componentType;
        getDataF = getComponent<OT>();
    }

    AccessorLoader(GLTF* model, GLTF::Accessor* accessor, GLTF::BufferView* bufferView, uint32_t accessorByteOffset, uint32_t componentType,
                   std::string type)
        : _accessor{accessor} {
        _bufferView = bufferView;
        uint32_t baseOffset = accessorByteOffset + _bufferView->byteOffset;
        stride = _bufferView->byteStride.has_value() ? _bufferView->byteStride.value() : sizeSwitch(componentType, type);
        _componentType = componentType;
        data = model->buffers[_bufferView->buffer].data.data();
        data += baseOffset;
        getDataF = getComponent<OT>();
    }

    OT at(uint32_t count_index) { return getDataF(data, count_index * stride); }

  private:
    unsigned char* data;
    GLTF::Accessor* _accessor;
    GLTF::BufferView* _bufferView;
    uint32_t stride;
    uint32_t _componentType = -1;
    OT (*getDataF)(unsigned char*, uint32_t);

    template <typename T> static OT getBufferData(unsigned char* data, uint32_t offset) {
        return static_cast<OT>(*reinterpret_cast<T*>(data + offset));
    }

    template <typename RT, typename T> struct AccessorLoaders {
        static auto getAccessorValue() { return getBufferData<T>; }
    };

    template <typename T> struct AccessorLoaders<glm::vec2, T> {
        static auto getAccessorValue() { return getBufferData<glm::vec<2, T>>; }
    };

    template <typename T> struct AccessorLoaders<glm::vec3, T> {
        static auto getAccessorValue() { return getBufferData<glm::vec<3, T>>; }
    };

    template <typename T> struct AccessorLoaders<glm::vec4, T> {
        static auto getAccessorValue() { return getBufferData<glm::vec<4, T>>; }
    };

    template <typename T> struct AccessorLoaders<glm::mat2, T> {
        static auto getAccessorValue() { return getBufferData<glm::mat<2, 2, T>>; }
    };
    template <typename T> struct AccessorLoaders<glm::mat3, T> {
        static auto getAccessorValue() { return getBufferData<glm::mat<3, 3, T>>; }
    };
    template <typename T> struct AccessorLoaders<glm::mat4, T> {
        static auto getAccessorValue() { return getBufferData<glm::mat<4, 4, T>>; }
    };

    template <typename RT> auto getComponent() {
        switch (_componentType) {
        case 5120:
            return AccessorLoaders<RT, char>::getAccessorValue();
            break;
        case 5121:
            return AccessorLoaders<RT, unsigned char>::getAccessorValue();
            break;
        case 5122:
            return AccessorLoaders<RT, short>::getAccessorValue();
            break;
        case 5123:
            return AccessorLoaders<RT, unsigned short>::getAccessorValue();
            break;
        case 5125:
            return AccessorLoaders<RT, uint32_t>::getAccessorValue();
            break;
        case 5126:
            return AccessorLoaders<RT, float>::getAccessorValue();
            break;
        default:
            throw std::runtime_error("Unknown component type: " + std::to_string(_componentType));
            break;
        }
    }

    template <typename T> size_t typeSwitch(std::string type) {
        if (type.compare("SCALAR") == 0) {
            return sizeof(T);
        } else if (type.compare("VEC2") == 0) {
            return sizeof(glm::vec<2, T>);
        } else if (type.compare("VEC3") == 0) {
            return sizeof(glm::vec<3, T>);
        } else if (type.compare("VEC4") == 0) {
            return sizeof(glm::vec<4, T>);
        } else if (type.compare("MAT2") == 0) {
            return sizeof(glm::mat<2, 2, T>);
        } else if (type.compare("MAT3") == 0) {
            return sizeof(glm::mat<3, 3, T>);
        } else if (type.compare("MAT4") == 0) {
            return sizeof(glm::mat<4, 4, T>);
        } else {
            throw std::runtime_error("Unknown type: " + type);
        }
    }

    size_t sizeSwitch(uint32_t componentType, std::string type) {
        switch (componentType) {
        case 5120:
            return typeSwitch<char>(type);
            break;
        case 5121:
            return typeSwitch<unsigned char>(type);
            break;
        case 5122:
            return typeSwitch<short>(type);
            break;
        case 5123:
            return typeSwitch<unsigned short>(type);
            break;
        case 5125:
            return typeSwitch<uint32_t>(type);
            break;
        case 5126:
            return typeSwitch<float>(type);
            break;
        default:
            throw std::runtime_error("Unknown component type: " + std::to_string(componentType));
            break;
        }
    }
};

#endif // ACCESSORLOADER_H_
