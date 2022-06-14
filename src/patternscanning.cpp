#include "pch.h"

#include "builtins.h"
#include "exception.h"
#include "memoryutils.h"
#include "patternbyte.h"
#include "patternscanning.h"
#include "simd.h"

auto XKLib::PatternScanning::searchInProcess(
  PatternByte& pattern,
  const Process& process,
  const std::function<
    auto(PatternByte&, const data_t, const std::size_t, const ptr_t)
      ->bool>& searchMethod) -> void
{
    const auto& mmap      = process.mmap();
    const auto& area_name = pattern.areaName();

    if (area_name.empty())
    {
        for (const auto& area : mmap.areas())
        {
            if (area->isReadable())
            {
                const auto area_read = area->readAligned<SIMD::value_t>();

                searchMethod(pattern,
                             view_as<data_t>(area_read.data()
                                             + sizeof(SIMD::value_t)),
                             area->size(),
                             area->begin<ptr_t>());
            }
        }
    }
    else
    {
        searchInProcessWithAreaName(pattern, process, area_name);
    }
}

auto XKLib::PatternScanning::searchInProcessWithAreaName(
  PatternByte& pattern,
  const Process& process,
  const std::string& areaName,
  const std::function<
    auto(PatternByte&, const data_t, const std::size_t, const ptr_t)
      ->bool>& searchMethod) -> void
{
    const auto& mmap = process.mmap();

    for (const auto& area : mmap.areas())
    {
        if (area->isReadable()
            and (area->name().find(areaName) != std::string::npos))
        {
            const auto area_read = area->readAligned<SIMD::value_t>();

            searchMethod(pattern,
                         view_as<data_t>(area_read.data()
                                         + sizeof(SIMD::value_t)),
                         area->size(),
                         area->begin<ptr_t>());
        }
    }
}

auto XKLib::PatternScanning::searchV1(PatternByte& pattern,
                                      const data_t data,
                                      const std::size_t size,
                                      const ptr_t baseAddress) -> bool
{
    if (size < sizeof(SIMD::value_t)
        or (size % sizeof(SIMD::value_t) != 0))
    {
        XKLIB_EXCEPTION("Size must be aligned to "
                        + std::to_string(sizeof(SIMD::value_t))
                        + " bytes");
    }

    /* prepare stuffs */
    auto&& matches                = pattern.matches();
    const auto old_matches_size   = matches.size();
    const auto& pattern_bytes     = pattern.bytes();
    const auto pattern_size       = pattern_bytes.size();
    const auto& simd_masks_values = pattern.simdMasksValues();

    auto start_data   = data;
    auto current_data = data;

    while (current_data + pattern_size <= data + size)
    {
        for (const auto& mask_value : simd_masks_values)
        {
            switch (mask_value.skip_type)
            {
                case PatternByte::SIMDMaskValue::ALL_UNKNOWN:
                {
                    /**
                     * We know that it is always the same amount of bytes
                     * in all cases due to pre-processing
                     */
                    break;
                }

                case PatternByte::SIMDMaskValue::ALL_KNOWN:
                {
                    if (not SIMD::CMPMask8bits(
                          SIMD::LoadUnaligned(
                            view_as<SIMD::value_t*>(current_data)),
                          mask_value.value))
                    {
                        goto skip;
                    }

                    break;
                }

                case PatternByte::SIMDMaskValue::MIXED:
                {
                    /* if ((value & mask) == pattern_value) */
                    if (not SIMD::CMPMask8bits(
                          SIMD::And(
                            SIMD::LoadUnaligned(
                              view_as<SIMD::value_t*>(current_data)),
                            mask_value.mask),
                          mask_value.value))
                    {
                        goto skip;
                    }

                    break;
                }
            }

            current_data += sizeof(SIMD::value_t);
        }

        matches.push_back(
          view_as<ptr_t>(view_as<std::uintptr_t>(baseAddress)
                         + view_as<std::uintptr_t>(start_data - data)));
    skip:
        start_data++;
        current_data = start_data;
    }

    return matches.size() != old_matches_size;
}

auto XKLib::PatternScanning::searchV2(PatternByte& pattern,
                                      const data_t data,
                                      const std::size_t size,
                                      const ptr_t baseAddress) -> bool
{
    const auto& pattern_bytes        = pattern.bytes();
    auto&& matches                   = pattern.matches();
    const auto old_matches_size      = matches.size();
    const auto pattern_size          = pattern_bytes.size();
    const auto& vec_organized_values = pattern.vecOrganizedValues();

    auto start_data   = data;
    auto current_data = data;

    while (current_data + pattern_size <= data + size)
    {
        for (const auto& organized_values : vec_organized_values)
        {
            for (const auto& byte : organized_values.bytes)
            {
                if (byte != *current_data)
                {
                    goto skip;
                }

                current_data++;
            }

            current_data += organized_values.skip_bytes;
        }

        matches.push_back(
          view_as<ptr_t>(view_as<std::uintptr_t>(baseAddress)
                         + view_as<std::uintptr_t>(start_data - data)));

    skip:
        start_data++;
        current_data = start_data;
    }

    return matches.size() != old_matches_size;
}

auto XKLib::PatternScanning::searchV3(PatternByte& pattern,
                                      const data_t data,
                                      const std::size_t size,
                                      const ptr_t baseAddress) -> bool
{
    if (size < sizeof(SIMD::value_t)
        or (size % sizeof(SIMD::value_t) != 0))
    {
        XKLIB_EXCEPTION("Size must be aligned to "
                        + std::to_string(sizeof(SIMD::value_t))
                        + " bytes");
    }

    auto&& matches                = pattern.matches();
    const auto old_matches_size   = matches.size();
    const auto& pattern_bytes     = pattern.bytes();
    const auto pattern_size       = pattern_bytes.size();
    const auto& simd_masks_values = pattern.simdMasksValues();

    auto start_data   = data + size - pattern_size;
    auto current_data = start_data;

    /**
     * Reversed Boyer Moore variant starts from the start of the pattern
     */
    auto it_mask_value = simd_masks_values.begin();

    /**
     * NOTE:
     * Way too much parameters to make it as a static method, lambda will
     * do fine, you could say that I could use just attributes but then I
     * would likely need to make instance, and this is not my view,
     * methods should stay static in pattern scanning.
     */
    const auto do_scan = [&](const auto& mismatch_byte_num,
                             const auto& mask,
                             const auto& value)
    {
        /* this part of the pattern mismatched ? */
        if (mismatch_byte_num > 0
            and mismatch_byte_num <= sizeof(SIMD::value_t))
        {
            /**
             * We got a mismatch, we need to re-align pattern /
             * adjust current data ptr
             */

            /* get the mismatched byte */
            const auto simd_tmp = SIMD::Set8bits(
              view_as<char>(*(current_data + (mismatch_byte_num - 1))));

            std::size_t to_skip;

            /**
             * Do no enter if the last character of that part is
             * mismatching
             */

            if (mismatch_byte_num < it_mask_value->part_size)
            {
                /**
                 * Get the matched byte with a second mask,
                 * due to the fact that we've already
                 * checked previous values before entering here
                 * with the previous cmp
                 */

                constexpr auto bit_mask = []() constexpr
                {
                    static_assert(sizeof(SIMD::value_t) <= 64,
                                  "SIMD::value_t is bigger than "
                                  "64 "
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

                const auto loaded_mask_value = SIMD::And(simd_tmp, mask);

                const auto compare_matched_bytes = SIMD::CMPMask8bits(
                                                     loaded_mask_value,
                                                     value)
                                                   & (std::numeric_limits<
                                                        std::uint64_t>::max()
                                                      << mismatch_byte_num)
                                                   & bit_mask;

                const auto match_byte_num = view_as<std::size_t>(
                  Builtins::FFS(compare_matched_bytes));

                if (match_byte_num > 0
                    and match_byte_num <= it_mask_value->part_size)
                {
                    to_skip = match_byte_num - mismatch_byte_num;
                    goto good_char;
                }
                else
                {
                    to_skip = it_mask_value->part_size - mismatch_byte_num
                              + 1;
                }
            }
            else
            {
                /* nothing to skip here */
                to_skip = 0;
            }

            /* pass on the next part */
            it_mask_value++;

            /* then search inside the rest of the pattern */
            while (it_mask_value != simd_masks_values.end())
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
                 * Matched, we found the mismatched byte inside
                 * the rest of the pattern
                 */
                if (match_byte_num > 0
                    and match_byte_num <= it_mask_value->part_size)
                {
                    to_skip += match_byte_num - 1;

                    /* exit */
                    goto good_char;
                }

                to_skip += it_mask_value->part_size;
                it_mask_value++;
            }

            to_skip++;

        good_char:
            /* otherwise just align pattern to the mismatch char */
            start_data -= to_skip;

            /* start from the beginning */
            it_mask_value = simd_masks_values.begin();

            /* apply new cursor position */
            current_data = start_data;
        }
        else
        {
            /* did we found our stuff ? */
            if (it_mask_value == std::prev(simd_masks_values.end()))
            {
                matches.push_back(view_as<ptr_t>(
                  view_as<std::uintptr_t>(baseAddress)
                  + view_as<std::uintptr_t>(start_data - data)));

                /* set new data cursor at data cursor - 1 */
                start_data--;
                current_data  = start_data;
                it_mask_value = simd_masks_values.begin();
            }
            else
            {
                /**
                 * Should be always a sizeof SIMD value, except last
                 * iterator
                 */
                current_data += sizeof(SIMD::value_t);
                it_mask_value++;
            }
        }
    };

    /**
     * Compare with our value (with unknown bytes, the mask)
     * Invert bits with cmp result and turn them into bits so we can
     * find the first set, so the mismatched byte.
     */

    while (current_data >= data + pattern_size)
    {
        switch (it_mask_value->skip_type)
        {
            case PatternByte::SIMDMaskValue::ALL_UNKNOWN:
            {
                /**
                 * We know that it is always the same amount of bytes in
                 * all cases due to pre-processing
                 */
                current_data += sizeof(SIMD::value_t);
                it_mask_value++;
                continue;
            }

            case PatternByte::SIMDMaskValue::ALL_KNOWN:
            {
                const auto loaded_value = SIMD::LoadUnaligned(
                  view_as<SIMD::value_t*>(current_data));

                const auto pattern_loaded_value = it_mask_value->value;

                const auto compare_mismatched_bytes = ~SIMD::CMPMask8bits(
                  loaded_value,
                  pattern_loaded_value);

                const auto mismatch_byte_num = view_as<std::size_t>(
                  Builtins::FFS(compare_mismatched_bytes));

                do_scan(mismatch_byte_num,
                        it_mask_value->mask,
                        pattern_loaded_value);
                break;
            }

            case PatternByte::SIMDMaskValue::MIXED:
            {
                const auto loaded_value = SIMD::LoadUnaligned(
                  view_as<SIMD::value_t*>(current_data));

                const auto pattern_loaded_value = it_mask_value->value;

                const auto mask = it_mask_value->mask;

                const auto loaded_mask_value = SIMD::And(loaded_value,
                                                         mask);

                const auto compare_mismatched_bytes = ~SIMD::CMPMask8bits(
                  loaded_mask_value,
                  pattern_loaded_value);

                const auto mismatch_byte_num = view_as<std::size_t>(
                  Builtins::FFS(compare_mismatched_bytes));

                do_scan(mismatch_byte_num, mask, pattern_loaded_value);
                break;
            }
        }
    }

    return matches.size() != old_matches_size;
}

auto XKLib::PatternScanning::searchV4(PatternByte& pattern,
                                      const data_t data,
                                      const std::size_t size,
                                      const ptr_t baseAddress) -> bool
{
    if (size < sizeof(SIMD::value_t)
        or (size % sizeof(SIMD::value_t) != 0))
    {
        XKLIB_EXCEPTION("Size must be aligned to "
                        + std::to_string(sizeof(SIMD::value_t))
                        + " bytes");
    }

    auto&& matches                  = pattern.matches();
    const auto old_matches_size     = matches.size();
    const auto& pattern_bytes       = pattern.bytes();
    const auto pattern_size         = pattern_bytes.size();
    const auto& simd_masks_values   = pattern.simdMasksValues();
    const auto& horspool_skip_table = pattern.horspoolSkipTable();

    auto start_data   = data + size - pattern_size;
    auto current_data = start_data;

    /**
     * Reversed Boyer Moore variant starts from the start of the pattern
     */
    auto it_mask_value = simd_masks_values.begin();

    /**
     * NOTE:
     * Way too much parameters to make it as a static method, lambda will
     * do fine, you could say that I could use just attributes but then I
     * would likely need to make instance, and this is not my view,
     * methods should stay static in pattern scanning.
     */
    const auto do_scan = [&](const auto& mismatch_byte_num)
    {
        /* this part of the pattern mismatched ? */
        if (mismatch_byte_num > 0
            and mismatch_byte_num <= sizeof(SIMD::value_t))
        {
            /**
             * We got a mismatch, we need to re-align pattern / adjust
             * current data ptr
             */
            const auto pattern_index = (current_data - start_data)
                                       + mismatch_byte_num - 1;

            /* use skip table instead, takes a lot of memory though */
            start_data -= horspool_skip_table[*(
              start_data + pattern_index)][pattern_index];

            /* apply new cursor position */
            current_data = start_data;

            /* start from the beginning */
            it_mask_value = simd_masks_values.begin();
        }
        else
        {
            /* did we found our stuff ? */
            if (it_mask_value == std::prev(simd_masks_values.end()))
            {
                matches.push_back(view_as<ptr_t>(
                  view_as<std::uintptr_t>(baseAddress)
                  + view_as<std::uintptr_t>(start_data - data)));

                /* set new data cursor at data cursor - 1 */
                start_data--;
                current_data  = start_data;
                it_mask_value = simd_masks_values.begin();
            }
            else
            {
                /**
                 * Should be always a sizeof SIMD value, except last
                 * iterator
                 */
                current_data += sizeof(SIMD::value_t);
                it_mask_value++;
            }
        }
    };

    while (current_data >= data + pattern_size)
    {
        /**
         * Compare with our value (with unknown bytes, the mask)
         * Invert bits with cmp result and turn them into bits so we can
         * find the first set, so the mismatched byte.
         */
        switch (it_mask_value->skip_type)
        {
            case PatternByte::SIMDMaskValue::ALL_UNKNOWN:
            {
                /**
                 * We know that it is always the same amount of bytes in
                 * all cases due to pre-processing
                 */
                current_data += sizeof(SIMD::value_t);
                it_mask_value++;
                continue;
            }

            case PatternByte::SIMDMaskValue::ALL_KNOWN:
            {
                const auto loaded_value = SIMD::LoadUnaligned(
                  view_as<SIMD::value_t*>(current_data));

                const auto compare_mismatched_bytes = ~SIMD::CMPMask8bits(
                  loaded_value,
                  it_mask_value->value);

                const auto mismatch_byte_num = view_as<std::size_t>(
                  Builtins::FFS(compare_mismatched_bytes));

                do_scan(mismatch_byte_num);
                break;
            }

            case PatternByte::SIMDMaskValue::MIXED:
            {
                const auto loaded_value = SIMD::LoadUnaligned(
                  view_as<SIMD::value_t*>(current_data));

                const auto loaded_mask_value = SIMD::And(
                  loaded_value,
                  it_mask_value->mask);

                const auto compare_mismatched_bytes = ~SIMD::CMPMask8bits(
                  loaded_mask_value,
                  it_mask_value->value);

                const auto mismatch_byte_num = view_as<std::size_t>(
                  Builtins::FFS(compare_mismatched_bytes));

                do_scan(mismatch_byte_num);
                break;
            }
        }
    }

    return matches.size() != old_matches_size;
}

auto XKLib::PatternScanning::searchAlignedV1(PatternByte& pattern,
                                             const data_t alignedData,
                                             const std::size_t size,
                                             const ptr_t baseAddress)
  -> bool
{
    if (size < sizeof(SIMD::value_t)
        or (size % sizeof(SIMD::value_t) != 0))
    {
        XKLIB_EXCEPTION("Size must be aligned to "
                        + std::to_string(sizeof(SIMD::value_t))
                        + " bytes");
    }

    if ((view_as<std::uintptr_t>(alignedData) % sizeof(SIMD::value_t))
        != 0)
    {
        XKLIB_EXCEPTION("Buffer is not aligned to "
                        + std::to_string(sizeof(SIMD::value_t))
                        + " bytes");
    }

    auto&& matches                        = pattern.matches();
    const auto old_matches_size           = matches.size();
    const auto& pattern_bytes             = pattern.bytes();
    const auto pattern_size               = pattern_bytes.size();
    const auto& shifted_simd_masks_values = pattern
                                              .shiftedSIMDMasksValues();
    const auto& horspool_skip_table = pattern.horspoolSkipTable();

    auto start_data              = alignedData + size - pattern_size;
    auto aligned_current_data    = MemoryUtils::Align(start_data,
                                                   sizeof(SIMD::value_t));
    auto shift_from_current_data = start_data - aligned_current_data;
    auto it_mask_value = shifted_simd_masks_values[shift_from_current_data]
                           .begin();

    /**
     * NOTE:
     * Way too much parameters to make it as a static method, lambda will
     * do fine, you could say that I could use just attributes but then I
     * would likely need to make instance, and this is not my view,
     * methods should stay static in pattern scanning.
     */
    const auto do_scan = [&](const auto& mismatch_byte_num)
    {
        /* this part of the pattern mismatched ? */
        if (mismatch_byte_num > 0
            and mismatch_byte_num <= sizeof(SIMD::value_t))
        {
            /**
             * We got a mismatch, we need to re-align pattern / adjust
             * current data ptr
             */

            const auto pattern_index = (aligned_current_data - start_data)
                                       + (mismatch_byte_num - 1);

            /**
             * Use skip table instead, takes a lot of memory though.
             * We also need to take into account the shift,
             * in case it the pattern was misaligned.
             * Correction:
             * Actually since mismatch_byte_num gives the correct
             * index, there's no need to.
             */
            start_data -= horspool_skip_table[*(
              start_data + pattern_index)][pattern_index];

            aligned_current_data = MemoryUtils::Align(
              start_data,
              sizeof(SIMD::value_t));

            shift_from_current_data = start_data - aligned_current_data;

            /* start from the beginning */
            it_mask_value = shifted_simd_masks_values[shift_from_current_data]
                              .begin();
        }
        else
        {
            /* did we found our stuff ? */
            if (
              it_mask_value
              == std::prev(
                shifted_simd_masks_values[shift_from_current_data].end()))
            {
                matches.push_back(view_as<ptr_t>(
                  view_as<std::uintptr_t>(baseAddress)
                  + view_as<std::uintptr_t>(start_data - alignedData)));

                /* set new data cursor at data cursor - 1 */
                start_data--;

                aligned_current_data = MemoryUtils::Align(
                  start_data,
                  sizeof(SIMD::value_t));

                shift_from_current_data = start_data
                                          - aligned_current_data;

                it_mask_value = shifted_simd_masks_values
                                  [shift_from_current_data]
                                    .begin();
            }
            else
            {
                aligned_current_data += sizeof(SIMD::value_t);
                it_mask_value++;
            }
        }
    };

    while (aligned_current_data >= alignedData + pattern_size)
    {
        switch (it_mask_value->skip_type)
        {
            case PatternByte::SIMDMaskValue::ALL_UNKNOWN:
            {
                /**
                 * We know that it is always the same amount of bytes in
                 * all cases due to pre-processing
                 */
                aligned_current_data += sizeof(SIMD::value_t);
                it_mask_value++;
                continue;
            }

            case PatternByte::SIMDMaskValue::ALL_KNOWN:
            {
                const auto loaded_value = SIMD::Load(
                  view_as<SIMD::value_t*>(aligned_current_data));

                const auto compare_mismatched_bytes = ~SIMD::CMPMask8bits(
                  loaded_value,
                  it_mask_value->value);

                const auto mismatch_byte_num = view_as<std::size_t>(
                  Builtins::FFS(compare_mismatched_bytes));

                do_scan(mismatch_byte_num);
                break;
            }

            case PatternByte::SIMDMaskValue::MIXED:
            {
                const auto loaded_value = SIMD::Load(
                  view_as<SIMD::value_t*>(aligned_current_data));

                const auto loaded_mask_value = SIMD::And(
                  loaded_value,
                  it_mask_value->mask);

                const auto compare_mismatched_bytes = ~SIMD::CMPMask8bits(
                  loaded_mask_value,
                  it_mask_value->value);

                const auto mismatch_byte_num = view_as<std::size_t>(
                  Builtins::FFS(compare_mismatched_bytes));

                do_scan(mismatch_byte_num);
                break;
            }
        }
    }

    return matches.size() != old_matches_size;
}
