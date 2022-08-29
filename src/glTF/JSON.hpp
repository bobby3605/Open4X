#ifndef JSON_H_
#define JSON_H_
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <variant>
#include <vector>

class JSONnode {
public:
  typedef std::variant<std::string, int, double, bool, std::vector<JSONnode>> valueType;
  JSONnode(std::ifstream &file);
  // jsonString must have no whitespace
  JSONnode(std::stringstream &jsonString);
  void print();
  const std::string key() { return _key; }
  const valueType value() { return _value; }
  JSONnode find(std::string);

private:
  std::string _key;
  valueType _value;
  const valueType parse(std::stringstream &jsonString);
  std::string getString(std::stringstream &jsonString);
  std::string getNum(std::stringstream &jsonString);
  // 0 for array or value, 1 for object
  bool type = 0;
};

#endif // JSON_H_
