#include "object_manager.hpp"
#include "common.hpp"
#include "model.hpp"
#include "object.hpp"
#include <algorithm>
#include <thread>

ObjectManager::ObjectManager() {
    // Using more threads than supported by the hardware would only slow things down
    size_t num_threads = std::min((unsigned int)new_settings->object_refresh_threads, std::thread::hardware_concurrency());
    _invalid_objects_slicer = new VectorSlicer<Object*>(_invalid_objects, num_threads, [](Object*& obj) { obj->refresh_instance_data(); });
}

ObjectManager::~ObjectManager() { delete _invalid_objects_slicer; }

size_t ObjectManager::add_object(Model* model) {
    _objects.emplace_back(model, &_invalid_objects, &_invalid_objects_mutex);
    return _objects.size() - 1;
}

void ObjectManager::remove_object(size_t const& object_id) { _objects.erase(_objects.begin() + object_id); }

Object* ObjectManager::get_object(size_t const& object_id) {
    if (object_id < _objects.size()) {
        return &_objects[object_id];
    } else {
        throw std::runtime_error("error object: " + std::to_string(object_id) + " doesn't exist!");
    }
}

size_t ObjectManager::object_count() { return _objects.size(); }

void ObjectManager::set_name(size_t const& object_id, std::string const& name) { _object_names[name] = object_id; }

Object* ObjectManager::get_object(std::string const& name) { return get_object(_object_names[name]); }

void ObjectManager::refresh_invalid_objects() {
    _invalid_objects_slicer->run();
    _invalid_objects.clear();
}

void ObjectManager::preallocate(size_t const& count) {
    _invalid_objects.reserve(count);
    _objects.reserve(count);
}
