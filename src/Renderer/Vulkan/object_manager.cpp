#include "object_manager.hpp"
#include "object.hpp"

ObjectManager::ObjectManager() {}
ObjectManager::~ObjectManager() {
    for (auto object : _objects) {
        delete object.second;
    }
}

void ObjectManager::add_object(std::string name, Model* model) {
    Object* object = new Object(model);
    _objects.insert({name, object});
}

void ObjectManager::remove_object(std::string name) {
    delete _objects.at(name);
    _objects.erase(name);
}
