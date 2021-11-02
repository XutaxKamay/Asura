#include <utility>

#include "patternscanning.h"
#include "process.h"

XKLib::PatternByte::Value::Value(int value) : value(value)
{
}

XKLib::PatternByte::PatternByte(std::vector<Value> values,
                                std::string areaName,
                                std::vector<ptr_t> matches)
 : _values(std::move(values)), _fast_values({}),
   _matches(std::move(matches)), _area_name(std::move(areaName))
{
    if (!isValid())
    {
        XKLIB_EXCEPTION("Invalid pattern.");
    }

    /* Convert to fast values for faster pattern scanning */
    auto pattern_size       = _values.size();
    fastval_t fastval       = 0;
    size_t index_of_fastval = 0;

    for (size_t index = 0; index < pattern_size;)
    {
        auto pattern_value = _values[index].value;

        if (pattern_value == Value::type_t::UNKNOWN)
        {
            if (index_of_fastval != 0)
            {
                FastValue fvalue { false,
                                   fastval,
                                   index_of_fastval,
                                   (1ull << (index_of_fastval * CHAR_BIT))
                                     - 1ull };
                _fast_values.push_back(fvalue);
                fastval          = 0;
                index_of_fastval = 0;
            }

            /* We got one already */
            index++;
            index_of_fastval++;

            /* Search for the next ones, if there is */
            for (; index < pattern_size; index++, index_of_fastval++)
            {
                pattern_value = _values[index].value;

                if (pattern_value != Value::type_t::UNKNOWN)
                {
                    break;
                }
            }

            FastValue fvalue { true, 0, index_of_fastval, 0 };
            _fast_values.push_back(fvalue);
            index_of_fastval = 0;
        }
        else
        {
            *view_as<byte_t*>(view_as<uintptr_t>(&fastval) + index_of_fastval) = view_as<
              byte_t>(pattern_value);

            index_of_fastval++;

            if (index_of_fastval >= sizeof(fastval_t))
            {
                FastValue fvalue {
                    false,
                    fastval,
                    sizeof(fastval_t),
                    std::numeric_limits<PatternByte::fastval_t>::max()
                };
                _fast_values.push_back(fvalue);
                index_of_fastval = 0;
                fastval          = 0;
            }

            index++;
        }
    }

    if (index_of_fastval != 0)
    {
        FastValue fvalue { false,
                           fastval,
                           index_of_fastval,
                           (1ull << (index_of_fastval * CHAR_BIT))
                             - 1ull };
        _fast_values.push_back(fvalue);
    }
}

auto XKLib::PatternByte::values() -> std::vector<Value>&
{
    return _values;
}

auto XKLib::PatternByte::fvalues()
  -> std::vector<XKLib::PatternByte::FastValue>&
{
    return _fast_values;
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
