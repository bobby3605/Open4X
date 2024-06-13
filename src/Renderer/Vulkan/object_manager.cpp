#include "object_manager.hpp"
#include "common.hpp"
#include "model.hpp"
#include "object.hpp"
#include <algorithm>
#include <thread>

ObjectManager::ObjectManager() : _mt(time(NULL)), _distribution(0, new_settings->rand_limit) {
    // Using more threads than supported by the hardware would only slow things down
    size_t num_threads = std::min((unsigned int)new_settings->object_refresh_threads, std::thread::hardware_concurrency());
    _invalid_objects_slicer =
        new VectorSlicer<Object*>(_invalid_objects, num_threads, [&](size_t i) { _invalid_objects[i]->refresh_instance_data(); });

    _bulk_objects_slicer = new VectorSlicer<Object*>(_objects, new_settings->object_bulk_create_threads, [&](size_t i) {
        // NOTE:
        // _objects has to be a std::vector<Object*>, otherwise bulk add will fail,
        // because objects can have varying sizes (ex: _instances gets alloced after the constructor)
        _objects[i] = new Object(_bulk_add_model, _invalid_objects, _invalid_objects_count);
    });
}

ObjectManager::~ObjectManager() { delete _invalid_objects_slicer; }

size_t ObjectManager::add_object(Model* model) {
    _objects.emplace_back(new Object(model, _invalid_objects, _invalid_objects_count));
    return _objects.size() - 1;
}

void ObjectManager::remove_object(size_t const& object_id) { _objects.erase(_objects.begin() + object_id); }

Object* ObjectManager::get_object(size_t const& object_id) {
    if (object_id < _objects.size()) {
        return _objects[object_id];
    } else {
        throw std::runtime_error("error object: " + std::to_string(object_id) + " doesn't exist!");
    }
}

size_t ObjectManager::object_count() { return _objects.size(); }

void ObjectManager::set_name(size_t const& object_id, std::string const& name) { _object_names[name] = object_id; }

Object* ObjectManager::get_object(std::string const& name) { return get_object(_object_names[name]); }

void ObjectManager::refresh_invalid_objects() {
    // TODO
    // mutex to block object from modifying invalid_objects
    _invalid_objects_slicer->run({.offset = 0, .size = _invalid_objects_count});
    _invalid_objects.clear();
    _invalid_objects_count = 0;
}

void ObjectManager::preallocate(size_t const& count) {
    _invalid_objects.reserve(count);
    _objects.resize(count);
}

void ObjectManager::create_n_objects(Model* model, size_t const& count) {
    auto prealloc_start_time = std::chrono::high_resolution_clock::now();
    model->preallocate(count);
    // NOTE:
    // _objects must be prealloced before bulk slicer add
    // resize must be used because insert/emplace are not thread safe,
    // even if reserve was called before (size and maybe something else because iterators are invalidated)
    // resize for Object* is fast, because it doesn't call the Object constructor
    size_t bulk_objects_offset = _objects.size();
    preallocate(count);
    std::cout << "prealloc time: "
              << std::chrono::duration<float, std::chrono::milliseconds::period>(std::chrono::high_resolution_clock::now() -
                                                                                 prealloc_start_time)
                     .count()
              << "ms" << std::endl;

    _bulk_add_model = model;
    auto object_creation_start_time = std::chrono::high_resolution_clock::now();
    _bulk_objects_slicer->run({.offset = bulk_objects_offset, .size = count});
    // FIXME:
    // distribution isn't thread safe, so it can't be in the bulk object slicer
    // pass a vector of positions like model
    for (size_t i = 0; i < _objects.size(); ++i) {
        _objects[i]->position({_distribution(_mt), _distribution(_mt), _distribution(_mt)});
    }
    std::cout << "object creation time: "
              << std::chrono::duration<float, std::chrono::milliseconds::period>(std::chrono::high_resolution_clock::now() -
                                                                                 object_creation_start_time)
                     .count()
              << "ms" << std::endl;
}
