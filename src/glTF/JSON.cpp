#include "JSON.hpp"

void JSONnode::print(uint whitespace) {
  switch (type) {
  case OBJECT:
    std::cout << std::setw(whitespace) << "{" << std::endl;
    whitespace += 4;
    for (auto node : nodeList) {
      node.print(whitespace);
      if (node.type != KEY) {
        std::cout << std::setw(whitespace) << "," << std::endl;
      }
    }
    whitespace -= 4;
    std::cout << std::setw(whitespace) << "}" << std::endl;
    break;
  case ARRAY:
    std::cout << std::setw(whitespace) << "[";
    for (auto node : nodeList) {
      node.print(2);
      if (node.type != KEY) {
        std::cout << std::setw(0) << ",";
      }
    }
    std::cout << std::setw(whitespace) << "]" << std::endl;
    break;
  case KEY:
    std::cout << std::setw(whitespace) << "\"" << string << "\""
              << " : ";
    nodeList.front().print(1);
    break;
  case STRING:
    std::cout << std::setw(whitespace) << "\"" << string << "\"" << std::endl;
    break;
  case INTEGER:
    std::cout << std::setw(whitespace) << iValue << std::endl;
    break;
  case FLOAT:
    std::cout << std::setw(whitespace) << fValue << std::endl;
    break;
  case BOOL:
    std::cout << std::setw(whitespace) << bValue << std::endl;
    break;
  case JSON_NULL:
    std::cout << std::setw(whitespace) << "null" << std::endl;
    break;
  }
}

JSONnode::JSONnode(std::ifstream &file) {
  char c;
  file.get(c);
  while (c == ' ' || c == '\n' || c == '\b' || c == '\r' || c == '\t' || c == '\a' || c == '\0') {
    file.get(c);
  }
  switch (c) {
  case '{':
    type = OBJECT;
    nodeList.push_back(JSONnode(file));
    // keep parsing elements until the closing bracket is met
    while (!nodeList.back().lastNode) {
      nodeList.push_back(JSONnode(file));
      // bad solution, but it works
      if (nodeList.back().commaNode) {
        nodeList.pop_back();
      }
    }
    // remove the closing bracket node
    nodeList.pop_back();
    break;
  case '[':
    type = ARRAY;
    nodeList.push_back(JSONnode(file));
    while (!nodeList.back().lastNode) {
      nodeList.push_back(JSONnode(file));
      if (nodeList.back().commaNode) {
        nodeList.pop_back();
      }
    }
    nodeList.pop_back();
    break;
  case '"':
    // parse a string until the next "
    // not currently supporting escaping a "
    while (file.get(c) && c != '"') {
      string.push_back(c);
    }
    file.get(c);
    while (c == ' ') {
      file.get(c);
    }
    // if the next non-blank character is a :, then it is a key
    if (c == ':') {
      type = KEY;
      nodeList.push_back(JSONnode(file));
    } else {
      // otherwise, it is a parsed string
      type = STRING;
    }
    break;
  case ',':
    commaNode = true;
    break;
  case ']':
    lastNode = true;
    break;
  case '}':
    lastNode = true;
    break;
  default:
    // store the potential float/integer/bool/null in a string for easy type checking and conversion
    string.push_back(c);
    while (file.get(c) && (c != ',' && c != ' ' && c != '\n' && c != '\b' && c != '\r' && c != '\t' && c != '\a')) {
      string.push_back(c);
    }
    // Find the type of the value by first checking if it's an int, then float
    try {
      iValue = std::stoi(string);
      type = INTEGER;
    } catch (std::exception &e) {
      try {
        iValue = std::stof(string);
        type = FLOAT;
        // if it's not a float or int
      } catch (std::exception &e) {
        // check if it's a bool or null
        if (string == "true") {
          type = BOOL;
          bValue = 1;

        } else if (string == "false") {
          type = BOOL;
          bValue = 0;
        } else if (string == "null") {
          type = JSON_NULL;
        } else {
          std::runtime_error("Parse error on: " + string);
        }
      }
    }
    break;
  }
}
