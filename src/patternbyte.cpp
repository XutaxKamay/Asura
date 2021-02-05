#include "patternbyte.h"

#include "patternscanning.h"

XLib::PatternByte::Value::Value(int value) : value(value)
{
}

XLib::PatternByte::PatternByte(std::vector<Value> values,
                               std::vector<ptr_t> matches)
 : _values(values), _matches(matches)
{
    if (!isValid())
    {
        throw PatternScanningException(std::string(CURRENT_CONTEXT)
                                       + "Invalid pattern.");
    }

    PatternScanning::searchInProcess(*this, Process::self());
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
