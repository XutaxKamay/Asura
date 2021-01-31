#include "pattern.h"

XLib::Pattern::Byte::Byte(int value) : value(value)
{
}

XLib::Pattern::Pattern(std::vector<Byte> bytes,
                       std::vector<ptr_t> matches)
 : _bytes(bytes), _matches(matches)
{
}

auto XLib::Pattern::bytes() -> std::vector<Byte>&
{
    return _bytes;
}

auto XLib::Pattern::matches() -> std::vector<ptr_t>&
{
    return _matches;
}

bool XLib::Pattern::isValid()
{
    for (auto&& byte : _bytes)
    {
        if (byte.value == Byte::type_t::INVALID)
        {
            return false;
        }
    }

    return true;
}
