#include "JSON.hpp"

void JSONnode::print() {
  if (key() != "") {
    std::cout << '"' << key() << '"' << ":";
  }
  // It won't let me do std::get<value().index()>(value())
  // Probably because the type isn't constant
  switch (value().index()) {
  case 0:
    std::cout << std::get<0>(value());
    break;
  case 1:
    std::cout << std::get<1>(value());
    break;
  case 2:
    std::cout << std::get<2>(value());
    break;
  case 3:
    std::cout << std::get<3>(value());
    break;
  case 4:
    std::cout << "{";
    std::vector<JSONnode> v = std::get<4>(value());
    for (JSONnode n : v) {
      n.print();
    }
    std::cout << "}";
    break;
  }
}
JSONnode::JSONnode(std::ifstream &file) {
  std::stringstream jsonString;
  char c;
  while (file.get(c)) {
    if (!isspace(c)) {
      jsonString << c;
    }
  }
  _value = parse(jsonString);
}

JSONnode::JSONnode(std::stringstream &jsonString) { _value = parse(jsonString); }

std::string JSONnode::getString(std::stringstream &jsonString) {
  char c;
  std::string string;
  // Parse string until close quote
  jsonString.get(c);
  while (c != '"') {
    string.push_back(c);
    jsonString.get(c);
  }
  return string;
}

std::string JSONnode::getNum(std::stringstream &jsonString) {
  // unget the switched token
  jsonString.unget();
  char c;
  std::string string;
  do {
    jsonString.get(c);
    string.push_back(c);
  } while (c != ',' && c != '}' && c != ']');
  // pop the token the number-string
  string.pop_back();
  // unget the last character for object or array processing
  jsonString.unget();
  return string;
}

// jsonString should not have any whitespace for now
const JSONnode::valueType JSONnode::parse(std::stringstream &jsonString) {
  valueType outputValue;
  char c;
  jsonString.get(c);
  if (c == '{') {
    std::vector<JSONnode> nodes;
    // Get all fields of object
    do {
      nodes.push_back(JSONnode(jsonString));
      jsonString.get(c);
    } while (c == ',');
    // Check if close bracket after parsing the object
    if (c != '}') {
      std::string err;
      err.append("Expected: } got: " + std::to_string(c) + "\n");
      std::runtime_error(err.c_str());
    }
    // This assignment has to be after filling the vector, otherwise it returns an empty vector
    outputValue = nodes;
  } else if (c == '[') {
    std::vector<JSONnode> nodes;
    do {
      nodes.push_back(JSONnode(jsonString));
      jsonString.get(c);
    } while (c == ',');
    if (c != ']') {
      std::string err;
      err.append("Expected: ] got: " + std::to_string(c) + "\n");
      std::runtime_error(err.c_str());
    }
    outputValue = nodes;
  } else if (c == '"') {
    std::string string = getString(jsonString);
    jsonString.get(c);
    // Have to unget here because a token gets skipped when called from string token and _value = parse();
    // Token gets skipped because both the top of if statement and string gets a token
    if (c == ',' || c == '{' || c == '}' || c == '[' || c == ']') {
      jsonString.unget();
    }
    if (c == ':') {
      // string key
      _key = string;
      _value = parse(jsonString);
      outputValue = _value;
    } else {
      // string value
      outputValue = string;
    }
  } else if (c == 't') {
    // skip r, u, e
    jsonString.seekg(3, std::ios_base::cur);
    outputValue = true;
  } else if (c == 'f') {
    jsonString.seekg(4, std::ios_base::cur);
    outputValue = false;
  } else if (c == 'n') {
    jsonString.seekg(3, std::ios_base::cur);
  } else {
    std::string string = getNum(jsonString);
    bool isFloat = false;
    for (char numberChar : string) {
      if (numberChar == '.')
        isFloat = true;
    }
    if (isFloat) {
      outputValue = std::stof(string);
    } else {
      outputValue = std::stoi(string);
    }
  }
  return outputValue;
}
