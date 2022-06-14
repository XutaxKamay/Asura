#include "pch.h"

#include "builtins.h"
#include "exception.h"
#include "patternbyte.h"
#include "patternscanning.h"
#include "process.h"

XKLib::PatternByte::Value::Value(int value) : value(value)
{
}

XKLib::PatternByte::PatternByte(const std::vector<Value> bytes_,
                                const std::string areaName,
                                const std::vector<ptr_t> matches)
 : _bytes(std::move(bytes_)),
   _matches(std::move(matches)),
   _area_name(std::move(areaName))
{
    if (not isValid())
    {
        XKLIB_EXCEPTION("Invalid pattern.");
    }

    /**
     *  Let's do some preprocessing now for different scanning systems
     */

    setupOrganizedValues();

    /**
     * Setup variant of horspool precalculated table,
     * due to unknown bytes, the skip_table is dependant of the pattern
     * index
     */

    setupSIMDMaskValues();
}

auto XKLib::PatternByte::setupOrganizedValues() -> void
{
    /* index of the byte in the pattern */
    std::size_t index = 0;
    /* count how many unknown bytes we got */
    std::size_t count_unknown_byte = 0;
    /* know when values are known or not for later */
    bool are_known_values = true;
    /* known bytes for org values */
    std::vector<byte_t> known_bytes;

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
            if (not are_known_values)
            {
                /* push back the last count of unknown_bytes */
                _vec_organized_values.push_back({ known_bytes,
                                                  count_unknown_byte });
                count_unknown_byte = 0;
                known_bytes.clear();
                are_known_values = true;
            }

            /* push back known value */
            known_bytes.push_back(view_as<byte_t>(byte.value));
        }

        index++;
    }

    /* was there still some known values left after an unknown byte */
    if (known_bytes.size())
    {
        /* if yes push it back */
        _vec_organized_values.push_back({ known_bytes, 0 });
    }
}

auto XKLib::PatternByte::setupSIMDMaskValues(
  simd_masks_values_t& simdMasksValues,
  std::vector<Value>& bytes) -> void
{
    /**
     * NOTE:
     * This only works because x86 is in little endian,
     * for others arch that has big endian, Boyer-Moore alg might not
     * work and needs byte reversing (starting at
     * sizeof(SIMD::value_t) - 1 instead)
     */

    /* index of the simd value */
    std::size_t byte_simd_index = 0;
    /* mask - values */
    SIMD::value_t simd_value {}, simd_mask {};

    for (const auto& byte : bytes)
    {
        if (byte.value != Value::UNKNOWN)
        {
            /**
             * copy pattern data to the simd value and create the
             * mask
             */
            view_as<byte_t*>(&simd_value)[byte_simd_index] = view_as<
              byte_t>(byte.value);
            view_as<byte_t*>(&simd_mask)[byte_simd_index] = 0xFF;
        }

        byte_simd_index++;

        /* do wo we got a full simd value here */
        if (byte_simd_index >= sizeof(SIMD::value_t))
        {
            simdMasksValues.push_back({ simd_mask,
                                        simd_value,
                                        sizeof(SIMD::value_t),
                                        SIMDMaskValue::MIXED });

            /* reset values */
            std::memset(&simd_value, 0, sizeof(simd_value));
            std::memset(&simd_mask, 0, sizeof(simd_mask));
            byte_simd_index = 0;
        }
    }
    /**
     * was there some left bytes that needs to be stored a simd value
     */
    if (byte_simd_index > 0)
    {
        simdMasksValues.push_back({ simd_mask,
                                    simd_value,
                                    byte_simd_index,
                                    SIMDMaskValue::MIXED });
    }

    for (auto&& mask_value : simdMasksValues)
    {
        const auto mmask = view_as<std::size_t>(
          SIMD::MoveMask8bits(mask_value.mask));

        if (mmask == 0)
        {
            mask_value.skip_type = SIMDMaskValue::ALL_UNKNOWN;
        }
        else if (mmask == SIMD::cmp_all)
        {
            mask_value.skip_type = SIMDMaskValue::ALL_KNOWN;
        }
    }
}

auto XKLib::PatternByte::setupSIMDMaskValues() -> void
{
    setupSIMDMaskValues(_simd_masks_values, _bytes);
    setupHorspoolTable(_horspool_skip_table, _simd_masks_values, _bytes);

#ifdef false
    decltype(_horspool_skip_table) sktest;
    do_horspool_skip_table_slow(sktest, _bytes);

    /**
     * Test
     */
    for (std::size_t i = 0; i < std::numeric_limits<char>::max() + 1; i++)
    {
        for (std::size_t j = 0; j < _bytes.size(); j++)
        {
            if (sktest[i][j] != _horspool_skip_table[i][j])
            {
                XKLIB_EXCEPTION("Fix your algorithm kamay");
            }
        }
    }
#endif

    /**
     * Shifted table skip table, for aligned searching in pattern
     * scanning, takes a lot of preprocessing and memory, but it is again
     * more faster.
     */

    _shifted_simd_masks_values[0] = _simd_masks_values;

    auto copied_bytes = _bytes;

    for (std::size_t i = 1; i < _shifted_simd_masks_values.size(); i++)
    {
        copied_bytes.insert(copied_bytes.begin(), Value::UNKNOWN);

        setupSIMDMaskValues(_shifted_simd_masks_values[i], copied_bytes);
    }
}

auto XKLib::PatternByte::setupHorspoolTable(
  horspool_table_t& horspoolTable,
  simd_masks_values_t& simdMasksValues,
  std::vector<Value>& bytes) -> void
{
#ifdef false

    for (std::size_t i = 0; i < horspoolTable.size() + 1; i++)
    {
        horspoolTable[i].resize(bytes.size());

        for (std::size_t j = 0; j < bytes.size(); j++)
        {
            /* Find the wanted byte further in the pattern */
            const auto it = std::find_if(bytes.begin() + j + 1,
                                         bytes.end(),
                                         [&i](const Value& value)
                                         {
                                             if (value.value == i
                                                 or value.value
                                                      == Value::UNKNOWN)
                                             {
                                                 return true;
                                             }

                                             return false;
                                         });

            /**
             * Good character skip until matched byte or
             * unknown byte
             */
            if (it != bytes.end())
            {
                horspoolTable[i][j] = it->index - j;
            }
            /* Bad character, skip the whole pattern */
            else
            {
                horspoolTable[i][j] = bytes.size() - j;
            }
        }
    }
#endif

    for (std::size_t i = 0; i < horspoolTable.size() + 1; i++)
    {
        horspoolTable[i].resize(bytes.size());

        /* Get the asked byte */
        const auto simd_tmp = SIMD::Set8bits(view_as<char>(i));

        for (std::size_t j = 0; j < bytes.size(); j++)
        {
            const auto part      = (j + 1) / sizeof(SIMD::value_t);
            const auto prev_part = j / sizeof(SIMD::value_t);
            const auto slot      = (j + 1) % sizeof(SIMD::value_t);
            const auto prev_slot = j % sizeof(SIMD::value_t);
            auto ptr_cur_skip    = &horspoolTable[i][j];

            auto it_mask_value = simdMasksValues.begin() + part;

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
                    static_assert(sizeof(SIMD::value_t) <= 64,
                                  "SIMD::value_t is bigger than 64 "
                                  "bytes");

                    if constexpr (sizeof(SIMD::value_t) == 64)
                    {
                        return std::numeric_limits<std::uint64_t>::max();
                    }
                    else if constexpr (sizeof(SIMD::value_t) < 64)
                    {
                        return (1ull << sizeof(SIMD::value_t)) - 1ull;
                    }
                }
                ();

                const auto loaded_mask_value = SIMD::And(
                  simd_tmp,
                  it_mask_value->mask);

                const auto compare_matched_bytes = SIMD::CMPMask8bits(
                                                     loaded_mask_value,
                                                     it_mask_value->value)
                                                   & (std::numeric_limits<
                                                        std::uint64_t>::max()
                                                      << slot)
                                                   & bit_mask;

                const auto match_byte_num = view_as<std::size_t>(
                  Builtins::FFS(compare_matched_bytes));

                if (match_byte_num > 0
                    and match_byte_num <= it_mask_value->part_size)
                {
                    *ptr_cur_skip = (match_byte_num - 1) - prev_slot;
                    goto good_char;
                }
                else
                {
                    *ptr_cur_skip = it_mask_value->part_size - prev_slot;
                }

                /* Pass on the next part */
                it_mask_value++;
            }
            else
            {
                *ptr_cur_skip = 1;
            }

            /* Then search inside the rest of the pattern */
            while (it_mask_value != simdMasksValues.end())
            {
                const auto loaded_mask_value = SIMD::And(
                  simd_tmp,
                  it_mask_value->mask);

                const auto compare_matched_bytes = SIMD::CMPMask8bits(
                  loaded_mask_value,
                  it_mask_value->value);

                const auto match_byte_num = view_as<std::size_t>(
                  Builtins::FFS(compare_matched_bytes));

                /**
                 * Matched, we found the (unknown ?) byte
                 * inside the rest of the pattern.
                 */
                if (match_byte_num > 0
                    and match_byte_num <= it_mask_value->part_size)
                {
                    *ptr_cur_skip += match_byte_num - 1;
                    /* exit */
                    goto good_char;
                }

                *ptr_cur_skip += it_mask_value->part_size;
                it_mask_value++;
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
  -> const std::vector<OrganizedValues>&
{
    return _vec_organized_values;
}

auto XKLib::PatternByte::simdMasksValues() const
  -> const std::vector<SIMDMaskValue>&
{
    return _simd_masks_values;
}

auto XKLib::PatternByte::horspoolSkipTable() -> const
  decltype(_horspool_skip_table)&
{
    return _horspool_skip_table;
}

auto XKLib::PatternByte::shiftedSIMDMasksValues() -> const
  decltype(_shifted_simd_masks_values)&
{
    return _shifted_simd_masks_values;
}

auto XKLib::PatternByte::matches() -> std::vector<ptr_t>&
{
    return _matches;
}

auto XKLib::PatternByte::scan(const Process& process) -> void
{
    PatternScanning::searchInProcess(*this, process);
}
