#include "JSON.hpp"

void JSONnode::print() {
  std::cout << '"' << key() << '"' << " : ";
  // Can't distinguish between object and array
  if (std::holds_alternative<std::vector<JSONnode>>(value())) {
    std::cout << "{" << std::endl;
    for (JSONnode n : std::get<std::vector<JSONnode>>(value())) {
      n.print();
    }
    std::cout << "}" << std::endl;
  } else {
    // It won't let me do std::get<value().index()>(value())
    // Probably because the type isn't constant
    switch (value().index()) {
    case 0:
      std::cout << std::get<0>(value()) << std::endl;
      break;
    case 1:
      std::cout << std::get<1>(value()) << std::endl;
      break;
    case 2:
      std::cout << std::get<2>(value()) << std::endl;
      break;
    case 3:
      std::cout << std::get<3>(value()) << std::endl;
      break;
    }
  }
}

void JSONnode::Tokenizer::print() {
  std::cout << "Tokens: " << std::endl;
  for (tokenTypes token : tokens) {
    std::string string;
    switch (token) {
    case OPENBRACKET:
      string = "{";
      break;
    case CLOSEBRACKET:
      string = "}";
      break;
    case OPENBRACE:
      string = "[";
      break;
    case CLOSEBRACE:
      string = "]";
      break;
    case COMMA:
      string = ",";
      break;
    case COLON:
      string = ":";
      break;
    case STRING:
      string = "string";
      break;
    case TRUE:
      string = "true";
      break;
    case FALSE:
      string = "false";
      break;
    case JSON_NULL:
      string = "null";
      break;
    case NUMBER:
      string = "number";
      break;
    }
    std::cout << string << std::endl;
  }
  std::cout << "Strings: " << std::endl;
  for (std::string string : strings) {
    std::cout << string << std::endl;
  }
}

std::string JSONnode::Tokenizer::getAlphaNum() {
  currAlphaNum++;
  return strings.at(currAlphaNum - 1);
}

void JSONnode::Tokenizer::getToken(tokenTypes &token) {
  currToken++;
  token = tokens.at(currToken - 1);
}

JSONnode::Tokenizer::Tokenizer(std::ifstream &file) {
  char c;
  // clear initial whitespace
  file.get(c);
  if (isspace(c)) {
    while (isspace(c)) {
      file.get(c);
    }
    // if no whitespace, unget the character
  } else {
    file.unget();
  }
  // parse into tokens
  while (file.get(c)) {
    while (isspace(c) && !file.eof()) {
      file.get(c);
    }
    if (file.eof())
      break;
    switch (c) {
    case '{':
      tokens.push_back(OPENBRACKET);
      break;
    case '}':
      tokens.push_back(CLOSEBRACKET);
      break;
    case '[':
      tokens.push_back(OPENBRACE);
      break;
    case ']':
      tokens.push_back(CLOSEBRACE);
      break;
    case ',':
      tokens.push_back(COMMA);
      break;
    case '"':
      tokens.push_back(STRING);
      getString(file);
      break;
    case ':':
      tokens.push_back(COLON);
      break;
    case 't':
      tokens.push_back(TRUE);
      // skip r,u,e
      file.seekg(3, std::ios_base::cur);
      break;
    case 'f':
      tokens.push_back(FALSE);
      file.seekg(4, std::ios_base::cur);
      break;
    case 'n':
      tokens.push_back(JSON_NULL);
      file.seekg(3, std::ios_base::cur);
      break;
    default:
      tokens.push_back(NUMBER);
      getNum(file);
    }
  }
}

void JSONnode::Tokenizer::getString(std::ifstream &file) {
  char c;
  std::string string;
  // Parse string until close quote
  file.get(c);
  while (c != '"') {
    string.push_back(c);
    file.get(c);
  }
  strings.push_back(string);
}

void JSONnode::Tokenizer::getNum(std::ifstream &file) {
  // unget the switched token
  file.unget();
  char c;
  std::string string;
  string.push_back(c);
  file.get(c);
  while (c != ',' && !isspace(c)) {
    string.push_back(c);
    file.get(c);
  }
  // if it ended on a comma, restore it file.get() for object and array parsing
  if (c == ',') {
    file.unget();
  }
  strings.push_back(string);
}

JSONnode::JSONnode(std::ifstream &file) {
  // tokenizer = std::shared_ptr<Tokenizer>(new Tokenizer(file));
  tokenizer = new Tokenizer(file);
  _value = parse();
  tokenizer->print();
}

JSONnode::JSONnode() { parse(); }

const JSONnode::valueType JSONnode::parse() {
  valueType outputValue;
  tokenTypes token;
  tokenizer->getToken(token);
  if (token == OPENBRACKET) {
    std::vector<JSONnode> nodes;
    outputValue = nodes;
    // Get all fields of object
    do {
      nodes.push_back(JSONnode());
      tokenizer->getToken(token);
    } while (token == COMMA);
    // Check if close bracket after parsing the object
    if (token != CLOSEBRACKET) {
      std::string err;
      err.append("Expected: " + std::to_string(CLOSEBRACKET) + " got: " + std::to_string(token) + "\n");
      std::runtime_error(err.c_str());
    }
    std::cout << "Object length: " << nodes.size() << std::endl;
  } else if (token == OPENBRACE) {
    std::vector<JSONnode> nodes;
    outputValue = nodes;
    do {
      nodes.push_back(JSONnode());
      tokenizer->getToken(token);
    } while (token == COMMA);
    if (token != CLOSEBRACE) {
      std::string err;
      err.append("Expected: " + std::to_string(CLOSEBRACE) + " got: " + std::to_string(token) + "\n");
      std::runtime_error(err.c_str());
    }
  } else if (token == STRING) {
    tokenizer->getToken(token);
    if (token == COLON) {
      // string key
      _key = tokenizer->getAlphaNum();
      _value = parse();
      // Don't need to set outputValue here since this should run when creating objects or arrays
    } else {
      // string or number value
      outputValue = tokenizer->getAlphaNum();
    }
  } else if (token == TRUE) {
    outputValue = true;

  } else if (token == FALSE) {
    outputValue = false;
  } else if (token == JSON_NULL) {
  } else if (token == NUMBER) {
    std::string string = tokenizer->getAlphaNum();
    try {
      outputValue = std::stoi(string);
    } catch (std::exception &e) {
      outputValue = std::stof(string);
    }
  } else {
    std::string err;
    err.append("Unexpected: " + std::to_string(token));
  }
  return outputValue;
}
