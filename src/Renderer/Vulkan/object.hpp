#ifndef OBJECT_H_
#define OBJECT_H_
#include "model.hpp"

class Object {
  public:
    Object(Model* model);
    ~Object();

  private:
    Model* _model;
};

#endif // OBJECT_H_
