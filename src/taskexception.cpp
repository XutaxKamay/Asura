#include "taskexception.h"

using namespace XLib;

TaskException::TaskException(const std::string& msg) : _msg(msg)
{
}

auto& TaskException::msg()
{
    return _msg;
}
