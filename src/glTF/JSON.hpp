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
  typedef std::variant<std::string, int, float, bool, std::vector<JSONnode>> valueType;
  JSONnode(std::ifstream &file);
  void print();
  const std::string key() { return _key; }
  const valueType value() { return _value; }

private:
  enum tokenTypes {
    OPENBRACE,
    CLOSEBRACE,
    OPENBRACKET,
    CLOSEBRACKET,
    COMMA,
    COLON,
    STRING,
    TRUE,
    FALSE,
    JSON_NULL,
    NUMBER
  };

  class Tokenizer {
  public:
    Tokenizer(std::ifstream &file);
    void getToken(tokenTypes &token);
    std::string getAlphaNum();
    void print();

  private:
    std::vector<tokenTypes> tokens;
    std::vector<std::string> strings;
    int currToken;
    int currAlphaNum;
    void getString(std::ifstream &file);
    void getNum(std::ifstream &file);
  };

  // std::shared_ptr<Tokenizer> tokenizer;
  Tokenizer *tokenizer;
  std::string _key;
  valueType _value;
  const valueType parse();
  JSONnode();
};

#endif // JSON_H_
