#include <utility>

#include "patternscanning.h"
#include "process.h"

XKLib::PatternByte::Value::Value(int value) : value(value)
{
}

XKLib::PatternByte::PatternByte(std::vector<Value> values,
                                std::string areaName,
                                std::vector<ptr_t> matches)
 : _values(std::move(values)), _matches(std::move(matches)),
   _area_name(std::move(areaName))
{
    if (!isValid())
    {
        XKLIB_EXCEPTION("Invalid pattern.");
    }
}

auto XKLib::PatternByte::values() -> std::vector<Value>&
{
    return _values;
}

auto XKLib::PatternByte::matches() -> std::vector<ptr_t>&
{
    return _matches;
}

auto XKLib::PatternByte::isValid() -> bool
{
    if (_values.size() == 0)
    {
        return 0;
    }

    for (auto&& byte : _values)
    {
        if (byte.value == Value::type_t::INVALID)
        {
            return false;
        }
    }

    /* ? xx xx ... */
    if (_values[0].value == Value::type_t::UNKNOWN)
    {
        return false;
    }

    /* xx xx ? */
    if (_values[_values.size() - 1].value == Value::type_t::UNKNOWN)
    {
        return false;
    }

    return true;
}

auto XKLib::PatternByte::scan(Process& process) -> void
{
    PatternScanning::searchInProcess(*this, process);
}

auto XKLib::PatternByte::areaName() -> std::string
{
    return _area_name;
}
