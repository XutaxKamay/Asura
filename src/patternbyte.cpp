#include "patternbyte.h"

#include "patternscanning.h"

XLib::PatternByte::Value::Value(int value) : value(value)
{
}

XLib::PatternByte::PatternByte(std::vector<Value> values,
                               std::string areaName,
                               std::vector<ptr_t> matches)
 : _values(values), _matches(matches), _area_name(areaName)
{
    if (!isValid())
    {
        throw PatternScanningException(std::string(CURRENT_CONTEXT)
                                       + "Invalid pattern.");
    }

    if (_area_name.empty())
    {
        PatternScanning::searchInProcess(*this, Process::self());
    }
    else
    {
        PatternScanning::searchInProcessWithAreaName(*this,
                                                     Process::self(),
                                                     _area_name);
    }
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
