#include "object_manager.hpp"
#include "object.hpp"

ObjectManager::ObjectManager(Buffer* instance_data_buffer) : _instance_data_buffer(instance_data_buffer) {}
ObjectManager::~ObjectManager() {
    for (auto object : _objects) {
        delete object.second;
    }
}

Object* ObjectManager::add_object(std::string name, Model* model) {
    // TODO
    // Don't pass _model_matrices_buffer into object
    Object* object = new Object(model, &invalid_callback, _instance_data_buffer);
    _objects.insert({name, object});
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

void ObjectManager::update_instance_data(glm::mat4*& dst) {
    while (!invalid_callback.empty()) {
        Object* object = invalid_callback.pop();
        object->update_instance_data();
        upload_instance_data(object->instance_data_offset(), object->instance_data());
    }
}

void ObjectManager::upload_instance_data(VkDeviceSize const& offset, std::vector<InstanceData> const& instance_data) {
    void* dst = (char*)_instance_data_buffer->data() + offset;
    std::memcpy(dst, instance_data.data(), instance_data.size());
}
