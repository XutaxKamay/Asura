#include "exception.h"

#include <utility>

XLib::Exception::Exception(std::string msg) : _msg(std::move(msg))
{
}

auto XLib::Exception::msg() -> const std::string&
{
    return _msg;
}
