#include "object_manager.hpp"
#include "model.hpp"
#include "object.hpp"

ObjectManager::ObjectManager(StackAllocator<GPUAllocator>* instances_sub_allocator) : _instances_sub_allocator(instances_sub_allocator) {}
ObjectManager::~ObjectManager() {
    for (auto object : _objects) {
        delete object.second;
    }
}

Object* ObjectManager::add_object(std::string name, Model* model) {
    // TODO
    // Don't pass _model_matrices_buffer into object
    Object* object = new Object(model, &invalid_callback, _instances_sub_allocator);
    _objects.insert(std::pair<std::string, Object*>{name, object});
    return object;
}

void ObjectManager::remove_object(std::string name) {
    Object* object = _objects.at(name);
    delete _objects.at(name);
    _objects.erase(name);
}

Object* ObjectManager::get_object(std::string name) {
    if (_objects.count(name) == 1) {
        return _objects.at(name);
    } else {
        throw std::runtime_error("error object: " + name + " doesn't exist!");
    }
}

void ObjectManager::update_instance_data() {
    while (!invalid_callback.empty()) {
        Object* object = invalid_callback.pop();
        object->update_instance_data();
        upload_instance_data(object->instance_data_allocs(), object->instance_data());
    }
}

void ObjectManager::upload_instance_data(std::vector<SubAllocation> const& instance_data_allocs,
                                         std::vector<InstanceData> const& instance_data) {
    for (size_t i = 0; i < instance_data_allocs.size(); ++i) {
        _instances_sub_allocator->write(instance_data_allocs[i], &instance_data[i], sizeof(InstanceData));
    }
}
