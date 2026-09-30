#include "./stream.h"
#include "./logger.h"
#include <fstream>
#include <iterator>
#include <string>


char Stream::yield() {
  if (it == end)
    return '\0';
  auto current = *it;
  ++it;
  return current;
}
bool Stream::is_eof() {
    return it == end;
}
Stream::Stream(std::string &f) : file(f), it(file), end() {
  if (!file.is_open())
    // Throws error
    Log("File is already open in another program...", LogType::Error);
};
