#include "pch.h"

#include "patternscanning.h"
#include "process.h"

XKLib::PatternByte::Value::Value(int value) : value(value)
{
}

XKLib::PatternByte::PatternByte(std::vector<Value> values,
                                std::string areaName,
                                std::vector<ptr_t> matches)
 : _bytes(std::move(values)), _matches(std::move(matches)),
   _area_name(std::move(areaName))
{
    if (!isValid())
    {
        XKLIB_EXCEPTION("Invalid pattern.");
    }

    /**
     *  Let's do some preprocessing now for different scanning systems
     */

    /* index of the byte in the pattern */
    std::size_t index = 0;
    /* index of the simd value */
    std::size_t byte_simd_index = 0;
    /* count how many unknown bytes we got */
    std::size_t count_unknown_byte = 0;
    /* organized values */
    bool are_known_values = true;
    /* known bytes for org values */
    std::vector<byte_t> known_bytes;
    /* mask - values */
    simd_value_t simd_value {}, simd_mask {};

    for (auto&& byte : _bytes)
    {
        byte.index = index;

        if (byte.value == Value::UNKNOWN)
        {
            if (are_known_values)
            {
                are_known_values = false;
            }

            count_unknown_byte++;
        }
        else
        {
            if (!are_known_values)
            {
                /* push back the last count of unknown_bytes */
                _vec_organized_values.push_back(
                  { known_bytes, count_unknown_byte });
                count_unknown_byte = 0;
                known_bytes.clear();
                are_known_values = true;
            }

            /* push back known value */
            known_bytes.push_back(view_as<byte_t>(byte.value));
            /**
             * copy pattern data to the simd value and create the
             * mask
             */
            view_as<byte_t*>(&simd_value)[byte_simd_index] = view_as<
              byte_t>(byte.value);
            view_as<byte_t*>(&simd_mask)[byte_simd_index] = 0xFF;
        }

        index++;
        byte_simd_index++;

        /* do wo we got a full simd value here */
        if (byte_simd_index >= sizeof(simd_value_t))
        {
            _fast_aligned_mvs.push_back(
              { simd_mask, simd_value, sizeof(simd_value_t) });

            /* reset values */
            std::memset(&simd_value, 0, sizeof(simd_value));
            std::memset(&simd_mask, 0, sizeof(simd_mask));
            byte_simd_index = 0;
        }
    }

    /* was there still some known values left after an unknown byte */
    if (known_bytes.size())
    {
        /* if yes push it back */
        _vec_organized_values.push_back({ known_bytes, 0 });
    }

    /**
     * was there some left bytes that needs to be stored a simd value
     */
    if (byte_simd_index > 0)
    {
        _fast_aligned_mvs.push_back(
          { simd_mask, simd_value, byte_simd_index });
    }

    /**
     * Setup variant of horspool precalculated table,
     * due to unknown bytes, the skip_table is dependant of the pattern
     * index
     */

    for (int i = 0; i < std::numeric_limits<byte_t>::max() + 1; i++)
    {
        skip_table[i].resize(_bytes.size());

        for (std::size_t j = 0; j < _bytes.size(); j++)
        {
            /* find the wanted byte further in the pattern */
            auto it = std::find_if(_bytes.begin() + j + 1,
                                   _bytes.end(),
                                   [&i](const Value& value)
                                   {
                                       if (value.value == i
                                           || value.value
                                                == Value::UNKNOWN)
                                       {
                                           return true;
                                       }

                                       return false;
                                   });

            /* good character skip until matched byte or unknown byte */
            if (it != _bytes.end())
            {
                skip_table[i][j] = it->index - j;
            }
            /* bad character, skip the whole pattern */
            else
            {
                skip_table[i][j] = _bytes.size() - j;
            }
        }
    }
}

auto XKLib::PatternByte::bytes() -> std::vector<Value>&
{
    return _bytes;
}

auto XKLib::PatternByte::matches() -> std::vector<ptr_t>&
{
    return _matches;
}

auto XKLib::PatternByte::isValid() -> bool
{
    if (_bytes.size() == 0)
    {
        return false;
    }

    for (auto&& byte : _bytes)
    {
        if (byte.value == Value::INVALID)
        {
            return false;
        }
    }

    /* xx xx ? */
    if (_bytes[_bytes.size() - 1].value == Value::UNKNOWN)
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

auto XKLib::PatternByte::vec_organized_values()
  -> std::vector<organized_values_t>&
{
    return _vec_organized_values;
}

auto XKLib::PatternByte::fast_aligned_mvs() -> std::vector<simd_mv_t>&
{
    return _fast_aligned_mvs;
}
