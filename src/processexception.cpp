#include "processexception.h"

using namespace XLib;

ProcessException::ProcessException(const std::string& msg) : _msg(msg)
{
}

auto& ProcessException::msg()
{
    return _msg;
}
