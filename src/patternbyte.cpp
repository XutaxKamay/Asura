#include <utility>

#include "patternscanning.h"
#include "process.h"

XLib::PatternByte::Value::Value(int value) : value(value)
{
}

XLib::PatternByte::PatternByte(std::vector<Value> values,
                               std::string areaName,
                               std::vector<ptr_t> matches)
 : _values(std::move(values)), _matches(std::move(matches)),
   _area_name(std::move(areaName))
{
    if (!isValid())
    {
        XLIB_EXCEPTION("Invalid pattern.");
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

auto XLib::PatternByte::isValid() -> bool
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

auto XLib::PatternByte::scan(const Process& process) -> void
{
    PatternScanning::searchInProcess(*this, process);
}

auto XLib::PatternByte::areaName() -> std::string
{
    return _area_name;
}
