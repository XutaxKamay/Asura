#include "bufferexception.h"

XLib::BufferException::BufferException(const std::string& msg) : _msg(msg)
{
}

auto& XLib::BufferException::msg()
{
    return _msg;
}
