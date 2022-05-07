#include "pch.h"

#include "patternscanning.h"
#include "process.h"
#include "simd.h"

XKLib::PatternByte::Value::Value(int value) : value(value)
{
}

XKLib::PatternByte::PatternByte(const std::vector<Value> bytes,
                                const std::string areaName,
                                const std::vector<ptr_t> matches)
 : _bytes(std::move(bytes)), _matches(std::move(matches)),
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
              { simd_mask, simd_value, sizeof(simd_value_t), false });

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
          { simd_mask, simd_value, byte_simd_index, false });
    }

    for (auto&& mv : _fast_aligned_mvs)
    {
        if (mm_movemask_epi8(mm_load_simd(&mv.mask)) == 0)
        {
            mv.can_skip = true;
        }
    }

    /**
     * Setup variant of horspool precalculated table,
     * due to unknown bytes, the skip_table is dependant of the pattern
     * index
     */

    for (int i = 0; i < std::numeric_limits<byte_t>::max() + 1; i++)
    {
        skip_table[i].resize(_bytes.size());

        /**
         * NOTE:
         * Essentially it does that faster:
         */

#if false
        /*  Find the wanted byte further in the pattern */
        auto it = std::find_if(_bytes.begin() + j + 1,
                               _bytes.end(),
                               [&i](const Value& value)
                               {
                                   if (value.value == i
                                       or value.value == Value::UNKNOWN)
                                   {
                                       return true;
                                   }

                                   return false;
                               });

        /**
         * Good character skip until matched byte or
         * unknown byte
         */
        if (it != _bytes.end())
        {
            skip_table[i][j] = it->index - j;
        }
        /* Bad character, skip the whole pattern */
        else
        {
            skip_table[i][j] = _bytes.size() - j;
        }
#endif

        /* Get the asked byte */
        const auto simd_tmp = mm_set_epi8_simd(view_as<char>(i));

        for (std::size_t j = 0; j < _bytes.size(); j++)
        {
            const auto part      = (j + 1) / sizeof(simd_value_t);
            const auto prev_part = j / sizeof(simd_value_t);
            const auto slot      = (j + 1) % sizeof(simd_value_t);
            const auto prev_slot = j % sizeof(simd_value_t);
            auto ptr_cur_skip    = &skip_table[i][j];

            auto it_mv = _fast_aligned_mvs.begin() + part;

            /**
             * Do no enter if we entered in a new part
             */
            if (prev_part == part)
            {
                /**
                 * Don't check previous values of <= j.
                 */
                constexpr auto bit_mask = []() constexpr
                {
                    static_assert(sizeof(simd_value_t) <= 64,
                                  "simd_value_t is bigger than 64 "
                                  "bytes");

                    if constexpr (sizeof(simd_value_t) == 64)
                    {
                        return std::numeric_limits<std::uint64_t>::max();
                    }
                    else if constexpr (sizeof(simd_value_t) < 64)
                    {
                        return (1ull << sizeof(simd_value_t)) - 1ull;
                    }
                }
                ();

                const auto match_byte_num = view_as<std::size_t>(
                  __builtin_ffsll(
                    mm_cmp_epi8_simd(mm_and_simd(simd_tmp, it_mv->mask),
                                     it_mv->value)
                    & (std::numeric_limits<std::uint64_t>::max() << slot)
                    & bit_mask));

                if (match_byte_num > 0
                    and match_byte_num <= it_mv->part_size)
                {
                    *ptr_cur_skip = (match_byte_num - 1) - prev_slot;
                    goto good_char;
                }
                else
                {
                    *ptr_cur_skip = it_mv->part_size - prev_slot;
                }

                /* Pass on the next part */
                it_mv++;
            }
            else
            {
                *ptr_cur_skip = 1;
            }

            /* Then search inside the rest of the pattern */
            while (it_mv != _fast_aligned_mvs.end())
            {
                const std::size_t match_byte_num = view_as<std::size_t>(
                  __builtin_ffsll(
                    mm_cmp_epi8_simd(mm_and_simd(simd_tmp, it_mv->mask),
                                     it_mv->value)));

                /**
                 * Matched, we found the (unknown ?) byte
                 * inside the rest of the pattern.
                 */
                if (match_byte_num > 0
                    and match_byte_num <= it_mv->part_size)
                {
                    *ptr_cur_skip += match_byte_num - 1;
                    /* exit */
                    goto good_char;
                }

                *ptr_cur_skip += it_mv->part_size;
                it_mv++;
            }

        good_char:;
        }
    }
}

auto XKLib::PatternByte::bytes() const -> const std::vector<Value>&
{
    return _bytes;
}

auto XKLib::PatternByte::isValid() const -> bool
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

auto XKLib::PatternByte::areaName() const -> const std::string&
{
    return _area_name;
}

auto XKLib::PatternByte::vecOrganizedValues() const
  -> const std::vector<organized_values_t>&
{
    return _vec_organized_values;
}

auto XKLib::PatternByte::fastAlignedMVs() const
  -> const std::vector<simd_mv_t>&
{
    return _fast_aligned_mvs;
}

auto XKLib::PatternByte::matches() -> std::vector<ptr_t>&
{
    return _matches;
}

auto XKLib::PatternByte::scan(const Process& process) -> void
{
    PatternScanning::searchInProcess(*this, process);
}
