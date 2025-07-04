#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#define STRONG_TYPE(Name, Type, ...)                                                                                                       \
    struct __VA_ARGS__ Name {                                                                                                              \
        Type value;                                                                                                                        \
        Name() = default;                                                                                                                  \
        Name(const Name&) = default;                                                                                                       \
        Name(Name&&) = default;                                                                                                            \
        Name& operator=(const Name&) = default;                                                                                            \
        Name& operator=(Name&&) = default;                                                                                                 \
        template <typename... Args> constexpr Name(Args&&... args) : value(std::forward<Args>(args)...) {}                                 \
        operator Type&() { return value; }                                                                                                 \
        operator const Type&() const { return value; }                                                                                     \
    };

STRONG_TYPE(Position, glm::vec3)
STRONG_TYPE(Rotation, glm::quat)
STRONG_TYPE(Scale, glm::vec3)
STRONG_TYPE(ObjectMatrix, glm::mat4, alignas(16))
