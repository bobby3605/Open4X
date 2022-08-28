#ifndef JSON_H_
#define JSON_H_
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <variant>
#include <vector>

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
  void get(tokenTypes &token);
  void unget();
  std::string getAlphaNum();
  int getCurrToken() { return currToken; }
  int getCurrAlphaNum() { return currAlphaNum; }
  void print();
  std::string tokenString(tokenTypes token, int tokenNum = -1);

private:
  std::vector<tokenTypes> tokens;
  std::vector<std::string> strings;
  int currToken;
  int currAlphaNum;
  void getString(std::ifstream &file);
  void getNum(std::ifstream &file);
};

class JSONnode {
public:
  typedef std::variant<std::string, int, double, bool, std::vector<JSONnode>> valueType;
  JSONnode(std::ifstream &file);
  void print();
  const std::string key() { return _key; }
  const valueType value() { return _value; }

private:
  std::shared_ptr<Tokenizer> tokenizer;
  // Tokenizer *tokenizer;
  std::string _key;
  valueType _value;
  const valueType parse();
  JSONnode(std::shared_ptr<Tokenizer> tokenizer);
};

#endif // JSON_H_
