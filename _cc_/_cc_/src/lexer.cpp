#include "./logger.h"
#include <fstream>
#include <iterator>


class Lexer {
private:
  std::ifstream file;
  std::istreambuf_iterator<char> it;
  std::istreambuf_iterator<char> end;

public:
  Lexer(std::string &f) : file(f), it(file), end() {
    if (!file.is_open())
        // Throws error
      Log("File is already open in another program...", LogType::Error);
  };
  char yield() {
      if (it == end) return '\0';
      auto current = *it;
      ++it;
      return current;
  };
};
