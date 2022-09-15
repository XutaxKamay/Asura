#include "pch.h"

#include "exception.h"

Asura::Exception::Exception(std::string msg)
 : _msg(std::move(msg))
{
}

auto Asura::Exception::msg() const -> const std::string&
{
    return _msg;
}
