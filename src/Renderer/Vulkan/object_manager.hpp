#ifndef OBJECT_MANAGER_H_
#define OBJECT_MANAGER_H_

#include "../../utils/utils.hpp"
#include "object.hpp"
#include <cstddef>
#include <glm/fwd.hpp>
#include <random>
#include <string>
#include <unordered_map>

class ObjectManager {
  public:
    ObjectManager();
    ~ObjectManager();
    // returns an object id
    size_t add_object(Model* model);
    void remove_object(size_t const& object_id);
    Object* get_object(size_t const& object_id);
    Object* get_object(std::string const& name);
    size_t object_count();
    void set_name(size_t const& object_id, std::string const& name);
    void refresh_invalid_objects();
    void preallocate(size_t const& count);
    void create_n_objects(Model* model, size_t const& count);

  private:
    std::vector<Object*> _objects;
    std::unordered_map<std::string, size_t> _object_names;
    std::vector<Object*> _invalid_objects;
    VectorSlicer<Object*>* _invalid_objects_slicer;
    VectorSlicer<Object*>* _bulk_objects_slicer;
    Model* _bulk_add_model;
    std::mutex _invalid_objects_mutex;

    std::mt19937 _mt;
    std::uniform_real_distribution<float> _distribution;
};

#endif // OBJECT_MANAGER_H_
