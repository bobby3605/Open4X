#include "common.hpp"
#include <fcntl.h>
#include <glm/gtx/quaternion.hpp>
#include <stdexcept>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vulkan/vulkan_core.h>

def_vk_ext_cpp(vkGetDescriptorSetLayoutSizeEXT);
def_vk_ext_cpp(vkGetDescriptorSetLayoutBindingOffsetEXT);
def_vk_ext_cpp(vkGetDescriptorEXT);
def_vk_ext_cpp(vkCmdBindDescriptorBuffersEXT);
def_vk_ext_cpp(vkCmdSetDescriptorBufferOffsets2EXT);

std::string get_file_extension(std::string file_path) {
    try {
        return file_path.substr(file_path.find_last_of(".") + 1);
    } catch (std::exception& e) {
        throw std::runtime_error("failed to get file extension of: " + file_path);
    }
}

std::string get_filename(std::string file_path) {
    try {
        auto pos = file_path.find_last_of("/\\") + 1;
        // returns npos if / is not found, would happen if there is no subdirectory
        return file_path.substr(pos == std::string::npos ? 0 : pos);
    } catch (std::exception& e) {
        throw std::runtime_error("failed to get filename of: " + file_path);
    }
}

std::string get_filename_no_ext(std::string file_path) {
    try {
        std::string file_name_with_ext = get_filename(file_path);
        return file_name_with_ext.substr(0, file_name_with_ext.find_last_of("."));
    } catch (std::exception& e) {
        throw std::runtime_error("failed to get file extension of: " + file_path);
    }
}

// TODO
// Find a faster way to extract the valid buffer usage flags
// Flags like indirect need to be removed before the map can work properly
VkDescriptorType usage_to_type(VkBufferUsageFlags usage_flags) {
    for (auto& flag_pair : usage_to_types) {
        if ((usage_flags & flag_pair.first) == flag_pair.first) {
            return flag_pair.second;
        }
    }
    // TODO
    // Print hex string
    throw std::runtime_error("Failed to find descriptor type for usage flags: " + std::to_string(usage_flags));
}

VkBufferUsageFlags type_to_usage(VkDescriptorType type) {
    for (auto& flag_pair : usage_to_types) {
        if (type == flag_pair.second) {
            return flag_pair.first;
        }
    }
    // TODO
    // Print hex string or constant name
    throw std::runtime_error("Failed to find buffer usage for descriptor type: " + std::to_string(type));
}

MMIO::MMIO(std::filesystem::path const& file_path, int oflag, size_t size) {
    _fd = open(file_path.c_str(), oflag, (mode_t)0600);
    if (_fd == -1) {
        throw std::runtime_error("failed to open file: " + file_path.string() + " errno: " + std::to_string(errno));
    }
    struct stat sb;
    fstat(_fd, &sb);
    _size = sb.st_size;
    // default 0 size when reading,
    // non-zero when writing
    if (size != 0) {
        _size = size;
    }
    int prot;
    if (oflag == O_RDONLY) {
        prot = PROT_READ;
    } else if (oflag & O_WRONLY || oflag & O_RDWR) {
        prot = PROT_WRITE | (oflag & O_RDWR ? PROT_WRITE : 0);
        if (ftruncate(_fd, _size) == -1) {
            close(_fd);
            throw std::runtime_error("failed to truncate file: " + file_path.string() + " errno: " + std::to_string(errno));
        }
    } else {
        throw std::runtime_error("MMIO unknown oflag: " + std::to_string(oflag));
    }
    _mapping = mmap(NULL, _size, prot, MAP_SHARED, _fd, 0);
    if (_mapping == MAP_FAILED) {
        close(_fd);
        throw std::runtime_error("failed to mmap file: " + file_path.string() + " errno: " + std::to_string(errno));
    }
}

MMIO::~MMIO() {
    munmap(_mapping, _size);
    close(_fd);
}

void fast_trs_matrix(glm::vec3 const& translation, glm::quat const& rotation, glm::vec3 const& scale, glm::mat4& trs) {
    // Manually copying and setting object matrix because multiplying mat4 is slow
    // translation
    trs[3][0] = translation.x;
    trs[3][1] = translation.y;
    trs[3][2] = translation.z;
    // rotation
    glm::mat3 rotation_3x3 = glm::toMat3(rotation);
    trs[0][0] = rotation_3x3[0][0];
    trs[0][1] = rotation_3x3[0][1];
    trs[0][2] = rotation_3x3[0][2];
    trs[1][0] = rotation_3x3[1][0];
    trs[1][1] = rotation_3x3[1][1];
    trs[1][2] = rotation_3x3[1][2];
    trs[2][0] = rotation_3x3[2][0];
    trs[2][1] = rotation_3x3[2][1];
    trs[2][2] = rotation_3x3[2][2];
    // scale
    trs[0][0] *= scale.x;
    trs[1][1] *= scale.y;
    trs[2][2] *= scale.z;

    trs[0][3] = 0;
    trs[1][3] = 0;
    trs[2][3] = 0;
    trs[3][3] = 1;
}
