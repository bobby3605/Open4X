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

void Tokenizer::print() {
  std::cout << "Tokens: " << std::endl;
  for (tokenTypes token : tokens) {
    std::cout << tokenString(token) << std::endl;
  }

  std::cout << "Strings: " << std::endl;
  for (std::string string : strings) {
    std::cout << string << std::endl;
  }
}

std::string Tokenizer::tokenString(tokenTypes token, int tokenNum) {
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
    if (tokenNum == -1) {
      string = "string";
    } else {
      string = strings.at(tokenNum);
    }
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
    if (tokenNum == -1) {
      string = "number";
    } else {
      string = strings.at(tokenNum);
    }
    break;
  }
  return string;
}

std::string Tokenizer::getAlphaNum() {
  currAlphaNum++;
  return strings.at(currAlphaNum - 1);
}

void Tokenizer::get(tokenTypes &token) {
  token = tokens.at(currToken);
  ++currToken;
  std::cout << "Getting token: " << tokenString(token) << std::endl;
}

void Tokenizer::unget() { --currToken; }

Tokenizer::Tokenizer(std::ifstream &file) {
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
    std::cout << "Switching: " << c << std::endl;
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
      std::cout << "Adding string: " << strings.back() << std::endl;
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
      std::cout << "Adding number: " << strings.back() << std::endl;
    }
  }
}

void Tokenizer::getString(std::ifstream &file) {
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

void Tokenizer::getNum(std::ifstream &file) {
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
  tokenizer = std::shared_ptr<Tokenizer>(new Tokenizer(file));
  // tokenizer = new Tokenizer(file);
  _value = parse();
}

JSONnode::JSONnode(std::shared_ptr<Tokenizer> tokenizer) : tokenizer{tokenizer} { parse(); }

const JSONnode::valueType JSONnode::parse() {
  valueType outputValue;
  tokenTypes token;
  tokenizer->get(token);
  std::cout << "Parsing token: " << tokenizer->tokenString(token) << std::endl;
  if (token == OPENBRACE) {
    std::vector<JSONnode> nodes;
    outputValue = nodes;
    // Get all fields of object
    do {
      nodes.push_back(JSONnode(tokenizer));
      tokenizer->get(token);
      std::cout << "Got token on open brace: " << tokenizer->tokenString(token) << std::endl;
    } while (token == COMMA);
    // Check if close bracket after parsing the object
    if (token != CLOSEBRACE) {
      std::string err;
      err.append("Expected: " + std::to_string(CLOSEBRACE) + " got: " + std::to_string(token) + "\n");
      std::runtime_error(err.c_str());
    }
  } else if (token == OPENBRACKET) {
    std::vector<JSONnode> nodes;
    outputValue = nodes;
    do {
      nodes.push_back(JSONnode(tokenizer));
      tokenizer->get(token);
    } while (token == COMMA);
    if (token != CLOSEBRACKET) {
      std::string err;
      err.append("Expected: " + std::to_string(CLOSEBRACKET) + " got: " + std::to_string(token) + "\n");
      std::runtime_error(err.c_str());
    }
  } else if (token == STRING) {
    tokenizer->get(token);
    if (token == COLON) {
      // string key
      _key = tokenizer->getAlphaNum();
      std::cout << "Found key: " << _key << std::endl;
      // Need to unget here because getToken in string and at the top of the if statement,
      // skips over the value
      tokenizer->unget();
      _value = parse();
      if (std::holds_alternative<std::string>(_value)) {
        std::cout << "String value: " << std::get<std::string>(_value) << std::endl;
      }
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
      outputValue = std::atoi(string.c_str());
    } catch (std::exception &e) {
      try {
        outputValue = std::atof(string.c_str());
      } catch (std::exception &e) {
        std::cout << "Failed to parse: $" << string << "$" << std::endl;
      }
    }
  } else {
    std::string err;
    err.append("Unexpected: " + std::to_string(token));
  }
  return outputValue;
}
