#include "./logger.h"
#include <iostream>
#include <string>


std::string tag_to_enum(LogType t) {
  switch (t) {
  case LogType::Default:
    return "         ";
  case LogType::Warning:
    return "[Warn]:  ";
  case LogType::Error:
    return "[Error]: ";
  }
}
void Log(std::string s, LogType l) {
  if (l == LogType::Error)
    throw std::runtime_error(tag_to_enum(l) + s);

  std::cout << tag_to_enum(l) << s;
}
