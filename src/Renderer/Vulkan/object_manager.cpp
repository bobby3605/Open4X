#include "object_manager.hpp"
#include "model.hpp"
#include "object.hpp"
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

ObjectManager::ObjectManager() {
    // Using more threads than supported by the hardware would only slow things down
    uint32_t num_threads = std::min((unsigned int)2, std::thread::hardware_concurrency());
    for (uint32_t i = 0; i < num_threads; ++i) {
        _threads.emplace_back([&] {
            while (!_stop_threads) {
                Object* obj = invalid_callback.pop();
                // wait until signaled to start,
                // and can acquire an object from the queue
                while (obj == nullptr) {
                    if (_stop_threads) {
                        return;
                    }
                    std::unique_lock<std::mutex> lock(_invalid_callback_mutex);
                    _invalid_callback_cond.wait(lock);
                    obj = invalid_callback.pop();
                }
                obj->refresh_instance_data();
            }
        });
    }
}

ObjectManager::~ObjectManager() {
    // wake up all threads and cause them to exit
    _stop_threads = true;
    _invalid_callback_cond.notify_all();
    for (auto& thread : _threads) {
        thread.join();
    }
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

size_t ObjectManager::object_count() { return _objects.size(); }

void ObjectManager::refresh_invalid_objects() {
    _invalid_callback_cond.notify_all();
    while (!invalid_callback.empty()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
