#ifndef JSON_H_
#define JSON_H_
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

class JSONnode {
public:
  enum Types { OBJECT, ARRAY, KEY, STRING, INTEGER, FLOAT, BOOL, JSON_NULL };
  std::vector<JSONnode> nodeList;
  std::string string;
  float fValue;
  int iValue;
  bool bValue;
  Types type;
  bool lastNode = false;
  bool commaNode = false;

  JSONnode(std::ifstream &file);
  void print(uint whitespace = 0);
};
#endif // JSON_H_
