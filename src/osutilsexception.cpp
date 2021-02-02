#include "osutilsexception.h"

XLib::OSUtilsException::OSUtilsException(const std::string& msg)
 : _msg(msg)
{
}

auto XLib::OSUtilsException::msg() -> const std::string&
{
    return _msg;
}
