#include "patternbyte.h"

XLib::PatternByte::Value::Value(int value) : value(value)
{
}

XLib::PatternByte::PatternByte(std::vector<Value> values,
                               std::vector<ptr_t> matches)
 : _values(values), _matches(matches)
{
}

auto XLib::PatternByte::values() -> std::vector<Value>&
{
    return _values;
}

auto XLib::PatternByte::matches() -> std::vector<ptr_t>&
{
    return _matches;
}

bool XLib::PatternByte::isValid()
{
    for (auto&& byte : _values)
    {
        if (byte.value == Value::type_t::INVALID)
        {
            return false;
        }
    }

    return true;
}
