#include <fstream>
#include <iterator>
#include <string>

class Stream {
private:
  std::ifstream file;
  std::istreambuf_iterator<char> it;
  std::istreambuf_iterator<char> end;

public:
  Stream(std::string &f);
  char yield();
  bool is_eof();
};
