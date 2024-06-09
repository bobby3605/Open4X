#include "object_manager.hpp"
#include "model.hpp"
#include "object.hpp"

ObjectManager::~ObjectManager() {
    for (auto object : _objects) {
        delete object.second;
    }
}

Object* ObjectManager::add_object(std::string name, Model* model) {
    // TODO
    // Don't pass _model_matrices_buffer into object
    Object* object = new Object(model, &invalid_callback);
    _objects.insert(std::pair<std::string, Object*>{name, object});
    return object;
}

void ObjectManager::remove_object(std::string name) {
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

void ObjectManager::refresh_instance_data() {
    while (!invalid_callback.empty()) {
        invalid_callback.pop()->refresh_instance_data();
    }
}

size_t ObjectManager::object_count() { return _objects.size(); }
