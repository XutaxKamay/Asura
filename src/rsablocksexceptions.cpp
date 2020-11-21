#include "rsablocksexceptions.h"

XLib::RSABlocksException::RSABlocksException(const std::string& msg)
 : _msg(msg)
{
}

auto XLib::RSABlocksException::msg() -> std::string&
{
    return _msg;
}
