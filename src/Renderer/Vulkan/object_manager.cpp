#include "object_manager.hpp"
#include "common.hpp"
#include "model.hpp"
#include "object.hpp"
#include <algorithm>
#include <thread>

ObjectManager::ObjectManager()
    // Using more threads than supported by the hardware would only slow things down
    : _num_threads(std::min((unsigned int)new_settings->object_refresh_threads, std::thread::hardware_concurrency())), _thread_semaphore(0),
      _thread_barrier(_num_threads + 1) {
    _vector_slices.reserve(_num_threads);
    for (uint32_t thread_id = 0; thread_id < _num_threads; ++thread_id) {
        _threads.emplace_back([&, thread_id] {
            while (true) {
                _thread_semaphore.acquire();
                if (_stop_threads) {
                    return;
                }
                VectorSlice& slice = _vector_slices[thread_id];
                size_t end_offset = slice.offset + slice.size;
                for (size_t i = slice.offset; i < end_offset; ++i) {
                    _invalid_objects[i]->refresh_instance_data();
                }
                _thread_barrier.arrive_and_wait();
            }
        });
    }
}

ObjectManager::~ObjectManager() {
    // wake up all threads and cause them to exit
    _stop_threads = true;
    _thread_semaphore.release(_num_threads);
    for (auto& thread : _threads) {
        thread.join();
    }
    for (auto object : _objects) {
        delete object.second;
    }
}

Object* ObjectManager::add_object(std::string name, Model* model) {
    Object* object = new Object(model, &_invalid_objects);
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
    size_t obj_count = _invalid_objects.size();
    // Only use extra threads if there's enough objects waiting to be processed
    if (obj_count < _num_threads) {
        for (auto& obj : _invalid_objects) {
            obj->refresh_instance_data();
        }
    } else {
        size_t slice_size = obj_count / _num_threads;
        size_t remainder = obj_count % _num_threads;
        size_t curr_offset = 0;
        for (size_t i = 0; i < _num_threads; ++i) {
            _vector_slices[i].size = slice_size;
            _vector_slices[i].offset = curr_offset;
            curr_offset += slice_size;
            if (i == _num_threads - 1) {
                _vector_slices[i].size += remainder;
            }
        }
        _thread_semaphore.release(_num_threads);
        _thread_barrier.arrive_and_wait();
        _invalid_objects.clear();
    }
}

void ObjectManager::preallocate(size_t count) {
    _invalid_objects.reserve(count);
    _objects.reserve(count);
}
