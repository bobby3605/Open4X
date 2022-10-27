#ifndef ACCESSORLOADER_H_
#define ACCESSORLOADER_H_
#include "GLTF.hpp"
#include <glm/glm.hpp>
#include <string>

template <typename OT> class AccessorLoader {
  public:
    AccessorLoader(GLTF* model, GLTF::Accessor* accessor);
    AccessorLoader(GLTF* model, GLTF::Accessor* accessor, GLTF::BufferView* bufferView, uint32_t accessorByteOffset, uint32_t componentType,
                   std::string type);
    OT at(uint32_t count_index);

  private:
    unsigned char* data;
    GLTF::Accessor* _accessor;
    GLTF::BufferView* _bufferView;
    uint32_t baseOffset;
    uint32_t multiplyOffset;
    uint32_t _componentType = -1;

    template <typename T> static T getBufferData(unsigned char* ptr, int offset) { return *reinterpret_cast<T*>(ptr + offset); }

    template <typename T> static glm::vec2 getVec2(unsigned char* ptr, int offset) {

        float x = getBufferData<T>(ptr, offset);
        float y = getBufferData<T>(ptr, offset + sizeof(T));
        return glm::vec2(x, y);
    }

    template <typename T> static glm::vec3 getVec3(unsigned char* ptr, int offset) {

        float x = getBufferData<T>(ptr, offset);
        float y = getBufferData<T>(ptr, offset + sizeof(T));
        float z = getBufferData<T>(ptr, offset + 2 * sizeof(T));
        return glm::vec3(x, y, z);
    }

    template <typename T> static glm::vec4 getVec4(unsigned char* ptr, int offset) {

        float x = getBufferData<T>(ptr, offset);
        float y = getBufferData<T>(ptr, offset + sizeof(T));
        float z = getBufferData<T>(ptr, offset + 2 * sizeof(T));
        float w = getBufferData<T>(ptr, offset + 3 * sizeof(T));
        return glm::vec4(x, y, z, w);
    }

    template <typename RT, typename T> struct AccessorLoaders {
        static RT getAccessorValue(unsigned char* ptr, int offset) { return getBufferData<T>(ptr, offset); }
    };

    template <typename T> struct AccessorLoaders<glm::vec2, T> {
        static glm::vec2 getAccessorValue(unsigned char* ptr, int offset) { return getVec2<T>(ptr, offset); }
    };

    template <typename T> struct AccessorLoaders<glm::vec3, T> {
        static glm::vec3 getAccessorValue(unsigned char* ptr, int offset) { return getVec3<T>(ptr, offset); }
    };

    template <typename T> struct AccessorLoaders<glm::vec4, T> {
        static glm::vec4 getAccessorValue(unsigned char* ptr, int offset) { return getVec4<T>(ptr, offset); }
    };

    template <typename T> struct AccessorLoaders<glm::mat2, T> {
        static glm::mat2 getAccessorValue(unsigned char* ptr, int offset) {
            glm::vec2 x = AccessorLoaders<glm::vec2, T>::getAccessorValue(ptr, offset);
            glm::vec2 y = AccessorLoaders<glm::vec2, T>::getAccessorValue(ptr, offset + 2 * sizeof(T));
            // glm matrix constructors for vectors go by column
            // the matrix data is being read by row
            // so the transpose needs to be returned
            return glm::transpose(glm::mat2(x, y));
        }
    };

    template <typename T> struct AccessorLoaders<glm::mat3, T> {
        static glm::mat3 getAccessorValue(unsigned char* ptr, int offset) {
            glm::vec3 x = AccessorLoaders<glm::vec3, T>::getAccessorValue(ptr, offset);
            glm::vec3 y = AccessorLoaders<glm::vec3, T>::getAccessorValue(ptr, offset + 3 * sizeof(T));
            glm::vec3 z = AccessorLoaders<glm::vec3, T>::getAccessorValue(ptr, offset + 2 * 3 * sizeof(T));
            return glm::transpose(glm::mat3(x, y, z));
        }
    };

    template <typename T> struct AccessorLoaders<glm::mat4, T> {
        static glm::mat4 getAccessorValue(unsigned char* ptr, int offset) {
            glm::vec4 x = AccessorLoaders<glm::vec4, T>::getAccessorValue(ptr, offset);
            glm::vec4 y = AccessorLoaders<glm::vec4, T>::getAccessorValue(ptr, offset + 4 * sizeof(T));
            glm::vec4 z = AccessorLoaders<glm::vec4, T>::getAccessorValue(ptr, offset + 2 * 4 * sizeof(T));
            glm::vec4 w = AccessorLoaders<glm::vec4, T>::getAccessorValue(ptr, offset + 3 * 4 * sizeof(T));
            return glm::transpose(glm::mat4(x, y, z, w));
        }
    };

    template <typename T> T getComponent(uint32_t offset) {
        switch (_componentType) {
        case 5120:
            return AccessorLoaders<T, char>::getAccessorValue(data, offset);
            break;
        case 5121:
            return AccessorLoaders<T, unsigned char>::getAccessorValue(data, offset);
            break;
        case 5122:
            return AccessorLoaders<T, short>::getAccessorValue(data, offset);
            break;
        case 5123:
            return AccessorLoaders<T, unsigned short>::getAccessorValue(data, offset);
            break;
        case 5125:
            return AccessorLoaders<T, uint32_t>::getAccessorValue(data, offset);
            break;
        case 5126:
            return AccessorLoaders<T, float>::getAccessorValue(data, offset);
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

    template <typename T> T loadAccessor(GLTF* model, GLTF::Accessor* accessor, int count_index) {
        GLTF::BufferView bufferView = model->bufferViews[accessor->bufferView.value()];
        int offset = accessor->byteOffset + bufferView.byteOffset +
                     count_index * (bufferView.byteStride.has_value() ? bufferView.byteStride.value()
                                                                      : sizeSwitch(accessor->componentType, accessor->type));
        return getComponent<T>(accessor->componentType, model->buffers[bufferView.buffer].data.data(), offset);
    }
};

template class AccessorLoader<glm::vec4>;
template class AccessorLoader<glm::vec3>;
template class AccessorLoader<glm::vec2>;
template class AccessorLoader<uint32_t>;
template class AccessorLoader<float>;
#endif // ACCESSORLOADER_H_
