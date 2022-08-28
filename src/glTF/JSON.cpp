#include "JSON.hpp"

void JSONnode::print() {
  std::cout << '"' << key() << '"' << " : ";
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
  case 4:
    std::cout << "{" << std::endl;
    std::vector<JSONnode> v = std::get<4>(value());
    for (JSONnode n : v) {
      n.print();
    }
    std::cout << "}" << std::endl;
    break;
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
    switch (c) {
    case '{':
      tokens.push_back(OPENBRACE);
      break;
    case '}':
      tokens.push_back(CLOSEBRACE);
      break;
    case '[':
      tokens.push_back(OPENBRACKET);
      break;
    case ']':
      tokens.push_back(CLOSEBRACKET);
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

  do {
    file.get(c);
    string.push_back(c);
  } while (c != ',' && !isspace(c));
  // pop the comma or space off the number-string
  string.pop_back();
  // push the number-string into the strings vector
  strings.push_back(string);
  // if it ended on a comma, restore it file.get() for object and array parsing
  if (c == ',') {
    file.unget();
  }
}

JSONnode::JSONnode(std::ifstream &file) {
  tokenizer = std::shared_ptr<Tokenizer>(new Tokenizer(file));
  _value = parse();
  std::vector<JSONnode> tmp = std::get<std::vector<JSONnode>>(_value);
  std::cout << "Size: " << tmp.size() << std::endl;
}

JSONnode::JSONnode(std::shared_ptr<Tokenizer> tokenizer) : tokenizer{tokenizer} { _value = parse(); }

const JSONnode::valueType JSONnode::parse() {
  valueType outputValue;
  tokenTypes token;
  tokenizer->get(token);
  if (token == OPENBRACE) {
    std::vector<JSONnode> nodes;
    // Get all fields of object
    do {
      nodes.push_back(JSONnode(tokenizer));
      tokenizer->get(token);
    } while (token == COMMA);
    // Check if close bracket after parsing the object
    if (token != CLOSEBRACE) {
      std::string err;
      err.append("Expected: " + std::to_string(CLOSEBRACE) + " got: " + std::to_string(token) + "\n");
      std::runtime_error(err.c_str());
    }
    // This assignment has to be after filling the vector, otherwise it returns an empty vector
    outputValue = nodes;
  } else if (token == OPENBRACKET) {
    std::vector<JSONnode> nodes;
    do {
      nodes.push_back(JSONnode(tokenizer));
      tokenizer->get(token);
    } while (token == COMMA);
    if (token != CLOSEBRACKET) {
      std::string err;
      err.append("Expected: " + std::to_string(CLOSEBRACKET) + " got: " + std::to_string(token) + "\n");
      std::runtime_error(err.c_str());
    }
    outputValue = nodes;
  } else if (token == STRING) {
    tokenizer->get(token);
    // Have to unget here because a token gets skipped when called from string token and _value = parse();
    // Token gets skipped because both the top of if statement and string gets a token
    if (token == COMMA || token == OPENBRACE || token == CLOSEBRACE || token == OPENBRACKET || token == CLOSEBRACKET) {
      tokenizer->unget();
    }
    if (token == COLON) {
      // string key
      _key = tokenizer->getAlphaNum();
      _value = parse();
      outputValue = _value;
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
  } else {
    std::string err;
    err.append("Unexpected: " + std::to_string(token));
  }
  return outputValue;
}
