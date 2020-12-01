#include "bufferexception.h"

XLib::BufferException::BufferException(const std::string& msg) : _msg(msg)
{
}

auto XLib::BufferException::msg() -> const std::string&
{
    return _msg;
}
