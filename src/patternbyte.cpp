#include <utility>

#include "patternscanning.h"
#include "process.h"
#include <cstring>

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
    auto pattern_size = _values.size();
    fastval_t fastval {};
    size_t index_of_fastval {};

    for (size_t index = 0; index < pattern_size;)
    {
        auto pattern_value = _values[index].value;

        if (pattern_value == Value::type_t::UNKNOWN)
        {
            if (index_of_fastval != 0)
            {
                auto max_bits = index_of_fastval * CHAR_BIT;
#ifndef __AVX512F__
                fastval_t mask = (1ull << max_bits) - 1ull;
#else
                std::array<int64_t, 8> e {};
                size_t ei = 0;

                for (; ei < 8 && max_bits >= sizeof(int64_t); ei++)
                {
                    *view_as<uint64_t*>(&e[ei]) = std::numeric_limits<
                      uint64_t>::max();
                    max_bits -= sizeof(int64_t);
                }

                if (max_bits > 0)
                {
                    *view_as<uint64_t*>(&e[ei]) = (1ull << max_bits)
                                                  - 1ull;
                }

                auto mask = _mm512_set_epi64(e[7],
                                             e[6],
                                             e[5],
                                             e[4],
                                             e[3],
                                             e[2],
                                             e[1],
                                             e[0]);
#endif

                _fast_values.push_back(
                  { false, fastval, index_of_fastval, mask });

                index_of_fastval = 0;
                std::memset(&fastval, 0, sizeof(fastval));
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

            FastValue fvalue { true, {}, index_of_fastval, {} };
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
#ifndef __AVX512F__
                fastval_t mask = std::numeric_limits<uint64_t>::max();
#else
                fastval_t mask {};
#endif

                _fast_values.push_back(
                  { false, fastval, sizeof(fastval_t), mask });

                index_of_fastval = 0;
                std::memset(&fastval, 0, sizeof(fastval));
            }

            index++;
        }
    }

    if (index_of_fastval != 0)
    {
        auto max_bits = index_of_fastval * CHAR_BIT;
#ifndef __AVX512F__
        fastval_t mask = (1ull << max_bits) - 1ull;
#else
        std::array<int64_t, 8> e {};
        size_t ei = 0;

        for (; ei < 8 && max_bits >= sizeof(int64_t); ei++)
        {
            *view_as<uint64_t*>(&e[ei]) = std::numeric_limits<
              uint64_t>::max();
            max_bits -= sizeof(int64_t);
        }

        if (max_bits > 0)
        {
            *view_as<uint64_t*>(&e[ei]) = (1ull << max_bits) - 1ull;
        }

        auto mask = _mm512_set_epi64(e[7],
                                     e[6],
                                     e[5],
                                     e[4],
                                     e[3],
                                     e[2],
                                     e[1],
                                     e[0]);
#endif
        _fast_values.push_back({ false, fastval, index_of_fastval, mask });
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
