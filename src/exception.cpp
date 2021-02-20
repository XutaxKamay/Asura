#include "exception.h"

XLib::Exception::Exception(const std::string& msg) : _msg(msg)
{
}

auto XLib::Exception::msg() -> const std::string&
{
    return _msg;
}
