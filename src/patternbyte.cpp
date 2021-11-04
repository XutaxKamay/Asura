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

    _unknown_values.reserve(_values.size());

    /* Allocate simd values in aligned way for optimizations */
    _simd_values = view_as<simd_value_t*>(
      std::aligned_alloc(sizeof(simd_value_t), _values.size()));

    auto left = _values.size() % sizeof(simd_value_t);

    if (left > 0)
    {
        left = sizeof(simd_value_t) - left;
    }

    _simd_values_size = _values.size() + left;

    size_t index                       = 0;
    size_t ukval_contigous_count       = 0;
    size_t index_since_contigous_count = 0;

    for (auto&& value : _values)
    {
        value->index = index;

        if (value->value == Value::UNKNOWN)
        {
            index_since_contigous_count = index;
            ukval_contigous_count++;

            if (ukval_contigous_count >= sizeof(simd_value_t))
            {
                _unknown_values.push_back(
                  { index_since_contigous_count / sizeof(simd_value_t),
                    index_since_contigous_count % sizeof(simd_value_t),
                    index_since_contigous_count,
                    ukval_contigous_count });
                ukval_contigous_count = 0;
            }
        }
        else
        {
            if (ukval_contigous_count > 0)
            {
                _unknown_values.push_back(
                  { index_since_contigous_count / sizeof(simd_value_t),
                    index_since_contigous_count % sizeof(simd_value_t),
                    index_since_contigous_count,
                    ukval_contigous_count });
                ukval_contigous_count = 0;
            }

            view_as<byte_t*>(_simd_values)[index] = view_as<byte_t>(
              value->value);
        }

        index++;
    }
}

XKLib::PatternByte::~PatternByte()
{
    std::free(_simd_values);
}

auto XKLib::PatternByte::values() -> std::vector<std::shared_ptr<Value>>&
{
    return _values;
}

auto XKLib::PatternByte::unknown_values() -> std::vector<unknown_value_t>&
{
    return _unknown_values;
}

auto XKLib::PatternByte::simd_values() -> simd_value_t*
{
    return _simd_values;
}

auto XKLib::PatternByte::simd_values_size() -> size_t&
{
    return _simd_values_size;
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
