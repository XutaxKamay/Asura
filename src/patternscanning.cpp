#include "pch.h"

#include "memoryutils.h"

#include "builtins.h"

#include "exception.h"

#include "patternscanning.h"

#include "patternbyte.h"

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
                const auto area_read = area->read<SIMD::value_t>();

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
            const auto area_read = area->read<SIMD::value_t>();

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
    auto&& matches              = pattern.matches();
    const auto old_matches_size = matches.size();
    const auto& pattern_bytes   = pattern.bytes();
    const auto pattern_size     = pattern_bytes.size();
    const auto& simd_mvs        = pattern.SIMDMVs();

    auto start_data   = data;
    auto current_data = data;

    while (current_data + pattern_size <= data + size)
    {
        for (const auto& mv : simd_mvs)
        {
            /**
             * Fast check if they are all unknown bytes
             * We have the guarantee that patterns always have known bytes
             * at the end
             */
            if (mv.skip_type == PatternByte::simd_mv_t::ALL_UNKNOWN)
            {
                /**
                 * We know that it is always the same amount of bytes in
                 * all cases due to pre-processing
                 */
                current_data += sizeof(SIMD::value_t);

                continue;
            }

            /* if ((value & mask) == pattern_value) */
            if (!SIMD::CMPMask8bits(
                  SIMD::And(SIMD::LoadUnaligned(
                              view_as<SIMD::value_t*>(current_data)),
                            mv.mask),
                  mv.value))
            {
                goto skip;
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

    auto&& matches              = pattern.matches();
    const auto old_matches_size = matches.size();
    const auto& pattern_bytes   = pattern.bytes();
    const auto pattern_size     = pattern_bytes.size();
    const auto& simd_mvs        = pattern.SIMDMVs();

    auto start_data   = data + size - pattern_size;
    auto current_data = start_data;

    /**
     * Reversed Boyer Moore variant starts from the start of the pattern
     */
    auto it_mv = simd_mvs.begin();

    while (current_data >= data + pattern_size)
    {
        /**
         * Compare with our value (with unknown bytes, the mask)
         * Invert bits with cmp result and turn them into bits so we can
         * find the first set, so the mismatched byte.
         */

        const auto do_scan =
          [&](const auto& mismatch_byte_num, const auto& mask)
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
                  view_as<char>(*(current_data + mismatch_byte_num - 1)));

                std::size_t to_skip;

                /**
                 * Do no enter if the last character of that part is
                 * mismatching
                 */

                if (mismatch_byte_num < it_mv->part_size)
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
                            return std::numeric_limits<
                              std::uint64_t>::max();
                        }
                        else if constexpr (sizeof(SIMD::value_t) < 64)
                        {
                            return (1ull << sizeof(SIMD::value_t)) - 1ull;
                        }
                    }
                    ();

                    const auto match_byte_num = view_as<std::size_t>(
                      Builtins::FFS(
                        SIMD::CMPMask8bits(SIMD::And(simd_tmp, mask),
                                           it_mv->value)
                        & (std::numeric_limits<std::uint64_t>::max()
                           << mismatch_byte_num)
                        & bit_mask));

                    if (match_byte_num > 0
                        and match_byte_num <= it_mv->part_size)
                    {
                        to_skip = match_byte_num - mismatch_byte_num;
                        goto good_char;
                    }
                    else
                    {
                        to_skip = it_mv->part_size - mismatch_byte_num;
                    }
                }
                else
                {
                    /* nothing to skip here */
                    to_skip = 0;
                }

                /* pass on the next part */
                it_mv++;

                /* then search inside the rest of the pattern */
                while (it_mv != simd_mvs.end())
                {
                    const auto match_byte_num = view_as<std::size_t>(
                      Builtins::FFS(SIMD::CMPMask8bits(
                        SIMD::And(simd_tmp, it_mv->mask),
                        it_mv->value)));

                    /**
                     * Matched, we found the mismatched byte inside
                     * the rest of the pattern
                     */
                    if (match_byte_num > 0
                        and match_byte_num <= it_mv->part_size)
                    {
                        to_skip += match_byte_num - 1;
                        /* exit */
                        goto good_char;
                    }

                    to_skip += it_mv->part_size;
                    it_mv++;
                }

                /**
                 * If bad, we skip past to the last mismatched char
                 * set new cursor before the mismatched char (skip +
                 * 1)
                 */
                to_skip++;

            good_char:
                /* otherwise just align pattern to the mismatch char */
                start_data -= to_skip;

                /* start from the beginning */
                it_mv = simd_mvs.begin();

                /* apply new cursor position */
                current_data = start_data;
            }
            else
            {
                /* did we found our stuff ? */
                if (it_mv == std::prev(simd_mvs.end()))
                {
                    matches.push_back(view_as<ptr_t>(
                      view_as<std::uintptr_t>(baseAddress)
                      + view_as<std::uintptr_t>(start_data - data)));

                    /* set new data cursor at data cursor - 1 */
                    start_data--;
                    current_data = start_data;
                    it_mv        = simd_mvs.begin();
                }
                else
                {
                    current_data += it_mv->part_size;
                    it_mv++;
                }
            }
        };

        switch (it_mv->skip_type)
        {
            case PatternByte::simd_mv_t::ALL_UNKNOWN:
            {
                /**
                 * We know that it is always the same amount of bytes in
                 * all cases due to pre-processing
                 */
                current_data += sizeof(SIMD::value_t);
                it_mv++;
                continue;
            }

            case PatternByte::simd_mv_t::ALL_KNOWN:
            {
                const auto mismatch_byte_num = view_as<std::size_t>(
                  Builtins::FFS(~SIMD::CMPMask8bits(
                    SIMD::LoadUnaligned(
                      view_as<SIMD::value_t*>(current_data)),
                    it_mv->value)));

                do_scan(mismatch_byte_num, it_mv->mask);
                break;
            }

            case PatternByte::simd_mv_t::MIXED:
            {
                const auto mask              = it_mv->mask;
                const auto mismatch_byte_num = view_as<std::size_t>(
                  Builtins::FFS(~SIMD::CMPMask8bits(
                    SIMD::And(SIMD::LoadUnaligned(
                                view_as<SIMD::value_t*>(current_data)),
                              mask),
                    it_mv->value)));

                do_scan(mismatch_byte_num, mask);
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

    auto&& matches              = pattern.matches();
    const auto old_matches_size = matches.size();
    const auto& pattern_bytes   = pattern.bytes();
    const auto pattern_size     = pattern_bytes.size();
    const auto& simd_mvs        = pattern.SIMDMVs();

    auto start_data   = data + size - pattern_size;
    auto current_data = start_data;

    /**
     * Reversed Boyer Moore variant starts from the start of the pattern
     */
    auto it_mv = simd_mvs.begin();

    while (current_data >= data + pattern_size)
    {
        /**
         * Compare with our value (with unknown bytes, the mask)
         * Invert bits with cmp result and turn them into bits so we can
         * find the first set, so the mismatched byte.
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

                const auto pattern_index = (start_data - current_data)
                                           + mismatch_byte_num - 1;

                /* get the mismatched byte */
                const auto misbyte = *(start_data + pattern_index);

                /* use skip table instead, takes a lot of memory though */
                start_data -= pattern.skip_table[misbyte][pattern_index];

                /* start from the beginning */
                it_mv = simd_mvs.begin();

                /* apply new cursor position */
                current_data = start_data;
            }
            else
            {
                /* did we found our stuff ? */
                if (it_mv == std::prev(simd_mvs.end()))
                {
                    matches.push_back(view_as<ptr_t>(
                      view_as<std::uintptr_t>(baseAddress)
                      + view_as<std::uintptr_t>(start_data - data)));

                    /* set new data cursor at data cursor - 1 */
                    start_data--;
                    current_data = start_data;
                    it_mv        = simd_mvs.begin();
                }
                else
                {
                    current_data += it_mv->part_size;
                    it_mv++;
                }
            }
        };

        switch (it_mv->skip_type)
        {
            case PatternByte::simd_mv_t::ALL_UNKNOWN:
            {
                /**
                 * We know that it is always the same amount of bytes in
                 * all cases due to pre-processing
                 */
                current_data += sizeof(SIMD::value_t);
                it_mv++;
                continue;
            }

            case PatternByte::simd_mv_t::ALL_KNOWN:
            {
                const auto mismatch_byte_num = view_as<std::size_t>(
                  Builtins::FFS(~SIMD::CMPMask8bits(
                    SIMD::LoadUnaligned(
                      view_as<SIMD::value_t*>(current_data)),
                    it_mv->value)));

                do_scan(mismatch_byte_num);
                break;
            }

            case PatternByte::simd_mv_t::MIXED:
            {
                const auto mismatch_byte_num = view_as<std::size_t>(
                  Builtins::FFS(~SIMD::CMPMask8bits(
                    SIMD::And(SIMD::LoadUnaligned(
                                view_as<SIMD::value_t*>(current_data)),
                              it_mv->mask),
                    it_mv->value)));

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

    const auto& pattern_bytes   = pattern.bytes();
    auto&& matches              = pattern.matches();
    const auto old_matches_size = matches.size();
    const auto pattern_size     = pattern_bytes.size();
    const auto& simd_mvs        = pattern.SIMDMVs();

    if ((view_as<std::uintptr_t>(alignedData) % sizeof(SIMD::value_t))
        != 0)
    {
        XKLIB_EXCEPTION("Buffer is not aligned to "
                        + std::to_string(sizeof(SIMD::value_t))
                        + " bytes");
    }

    auto start_data   = alignedData;
    auto current_data = alignedData;

    /**
     * Here we are searching for a pattern that was aligned memory
     * on smid_value_t bits. This is really faster than the previous
     * methods.
     */
    while (current_data + pattern_size <= alignedData + size)
    {
        for (const auto& mv : simd_mvs)
        {
            /**
             * Fast check if they are all unknown bytes
             * We have the guarantee that patterns always have known bytes
             * at the end
             */
            if (mv.skip_type == PatternByte::simd_mv_t::ALL_UNKNOWN)
            {
                /**
                 * We know that it is always the same amount of bytes in
                 * all cases due to pre-processing
                 */
                current_data += sizeof(SIMD::value_t);
                continue;
            }

            if (!SIMD::CMPMask8bits(
                  SIMD::And(SIMD::Load(
                              view_as<SIMD::value_t*>(current_data)),
                            mv.mask),
                  mv.value))
            {
                goto skip;
            }

            current_data += sizeof(SIMD::value_t);
        }

        matches.push_back(view_as<ptr_t>(
          view_as<std::uintptr_t>(baseAddress)
          + view_as<std::uintptr_t>(start_data - alignedData)));

    skip:
        start_data += sizeof(SIMD::value_t);
        current_data = start_data;
    }

    return matches.size() != old_matches_size;
}

auto XKLib::PatternScanning::searchAlignedV2(PatternByte& pattern,
                                             const data_t alignedData,
                                             const std::size_t size,
                                             const ptr_t /*baseAddress*/)
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

    /**
     * TODO:
     */

    auto&& matches              = pattern.matches();
    const auto old_matches_size = matches.size();

    return matches.size() != old_matches_size;
}
