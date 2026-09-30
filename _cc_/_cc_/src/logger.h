#include <string>
enum class LogType {
    Default, Warning, Error
};
void Log(std::string s, LogType l);
