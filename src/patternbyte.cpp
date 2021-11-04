#include <utility>

#include "patternscanning.h"
#include "process.h"

#include <cstdlib>
#include <cstring>

XKLib::PatternByte::Value::Value(int value) : value(value)
{
}

XKLib::PatternByte::PatternByte(std::vector<std::shared_ptr<Value>> values,
                                std::string areaName,
                                std::vector<ptr_t> matches)
 : _values(std::move(values)), _matches(std::move(matches)),
   _area_name(std::move(areaName))
{
    if (!isValid())
    {
        XKLIB_EXCEPTION("Invalid pattern.");
    }

    _simd_values.reserve(_values.size() / sizeof(simd_value_t));
    _unknown_values.reserve(_values.size());

    size_t index = 0;
    for (auto&& value : _values)
    {
        value->index = index;

        if (value->value == Value::UNKNOWN)
        {
            _unknown_values.push_back(index);
        }

        index++;
    }
}

auto XKLib::PatternByte::values() -> std::vector<std::shared_ptr<Value>>&
{
    return _values;
}

auto XKLib::PatternByte::unknown_values() -> std::vector<unknown_value_t>&
{
    return _unknown_values;
}

auto XKLib::PatternByte::simd_values() -> std::vector<simd_value_t>&
{
    return _simd_values;
}

auto XKLib::PatternByte::aligned_simd_values()
  -> std::vector<XKLib::PatternByte::simd_value_t>
{
}

auto XKLib::PatternByte::matches() -> std::vector<ptr_t>&
{
    return _matches;
}

auto XKLib::PatternByte::isValid() -> bool
{
    if (_values.size() == 0)
    {
        return false;
    }

    for (auto&& byte : _values)
    {
        if (byte->value == Value::INVALID)
        {
            return false;
        }
    }

    /* ? xx xx ... */
    if (_values[0]->value == Value::UNKNOWN)
    {
        return false;
    }

    /* xx xx ? */
    if (_values[_values.size() - 1]->value == Value::UNKNOWN)
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
