#include "memoryexception.h"

using namespace XLib;

MemoryException::MemoryException(const std::string& msg) : _msg(msg)
{
}

auto MemoryException::msg() -> std::string&
{
    return _msg;
}
