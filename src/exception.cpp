#include "pch.h"

#include "exception.h"

XKLib::Exception::Exception(std::string msg) : _msg(std::move(msg))
{
}

auto XKLib::Exception::msg() -> const std::string&
{
    return _msg;
}
