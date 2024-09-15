#ifndef LIGHT_H_
#define LIGHT_H_
#include "object.hpp"

class Light : public Object {
  public:
    Light(ContiguousFixedAllocator<GPUAllocation>* light_allocator, Model* model, safe_vector<Object*>& invalid_objects);
    ~Light();
    glm::vec3 const& color() { return _light_data.color; }
    void color(glm::vec3 const& new_color);
    void position(glm::vec3 const& new_position);
    glm::vec3 const& position() { return _position; }

    struct GPULight {
        glm::vec3 position{0.0f, 0.0f, 0.0f};
        uint32_t pad0;
        glm::vec3 color{1.0f, 1.0f, 1.0f};
        uint32_t pad1;
    };

  private:
    std::string _light_buffer_name;
    ContiguousFixedAllocator<GPUAllocation>* _light_allocator;
    SubAllocation<ContiguousFixedAllocator, GPUAllocation>* _light_alloc;
    GPULight _light_data{};
    void write_light();
};

#endif // LIGHT_H_
