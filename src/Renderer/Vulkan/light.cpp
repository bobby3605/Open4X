#include "light.hpp"

Light::Light(ContiguousFixedAllocator<GPUAllocation>* light_allocator, Model* model, safe_vector<Object*>& invalid_objects,
             LinearAllocator<GPUAllocation>* object_instance_ids_allocator,
             ContiguousFixedAllocator<GPUAllocation>* object_culling_data_allocator)
    : Object(model, invalid_objects, object_instance_ids_allocator, object_culling_data_allocator), _light_allocator(light_allocator) {
    _light_alloc = _light_allocator->alloc();
    write_light();
    refresh_instance_data();
}

Light::~Light() { _light_allocator->pop_and_swap(_light_alloc); }

void Light::color(glm::vec3 const& new_color) {
    _light_data.color = new_color;
    write_light();
    refresh_instance_data();
}
void Light::position(glm::vec3 const& new_position) {
    Object::position(new_position);
    _light_data.position = new_position;
    write_light();
    refresh_instance_data();
}

void Light::write_light() { _light_alloc->write(&_light_data); }
