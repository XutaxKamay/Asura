#include "pch.h"

#include "patternscanning.h"

#include "patternbyte.h"

#if defined(__AVX512F__)
    #define _mm_cmp_pi8_simd_value(mm1, mm2)                             \
        _mm512_cmpeq_epi8_mask(mm1, mm2)
#elif defined(__AVX2__)
    #define _mm_cmp_pi8_simd_value(mm1, mm2)                             \
        _mm256_movemask_epi8(_mm256_cmpeq_epi8(mm1, mm2))
#else
    #define _mm_cmp_pi8_simd_value(mm1, mm2)                             \
        _mm_movemask_pi8(_mm_cmpeq_pi8(mm1, mm2))
#endif

#if defined(__AVX512F__)
    #define _mm_and_simd_value(mm1, mm2) _mm512_and_si512(mm1, mm2)
#elif defined(__AVX2__)
    #define _mm_and_simd_value(mm1, mm2) _mm256_and_si256(mm1, mm2)
#else
    #define _mm_and_simd_value(mm1, mm2) _mm_and_si64(mm1, mm2)
#endif

#if defined(__AVX512F__)
    #define _mm_load_simd_value(mm1)                                     \
        _mm512_load_si512(view_as<PatternByte::simd_value_t*>(mm1))
#elif defined(__AVX2__)
    #define _mm_load_simd_value(mm1)                                     \
        _mm256_load_si256(view_as<PatternByte::simd_value_t*>(mm1))
#else
    #define _mm_load_simd_value(mm1)                                     \
        _mm_cvtsi64_m64(*view_as<uint64_t*>(mm1))
#endif

#if defined(__AVX512F__)
    #define _mm_loadu_simd_value(mm1)                                    \
        _mm512_loadu_si512(view_as<PatternByte::simd_value_t*>(mm1))
#elif defined(__AVX2__)
    #define _mm_loadu_simd_value(mm1)                                    \
        _mm256_loadu_si256(view_as<PatternByte::simd_value_t*>(mm1))
#else
    #define _mm_loadu_simd_value(mm1)                                    \
        _mm_cvtsi64_m64(*view_as<uint64_t*>(mm1))
#endif

#if defined(__AVX512F__)
    #define _mm_set_pi8_simd_value(xx) _mm512_set1_epi8(xx)
#elif defined(__AVX2__)
    #define _mm_set_pi8_simd_value(xx) _mm256_set1_epi8(xx)
#else
    #define _mm_set_pi8_simd_value(xx) _mm_set1_pi8(xx)
#endif

auto XKLib::PatternScanning::searchV1(XKLib::PatternByte& pattern,
                                      data_t data,
                                      std::size_t size,
                                      ptr_t baseAddress) -> bool
{
    /* prepare stuffs */
    auto&& matches          = pattern.matches();
    auto old_matches_size   = matches.size();
    auto&& pattern_bytes    = pattern.bytes();
    auto pattern_size       = pattern_bytes.size();
    auto&& fast_aligned_mvs = pattern.fast_aligned_mvs();

    data_t start_data   = data;
    data_t current_data = data;

    while (current_data + pattern_size <= data + size)
    {
        for (auto&& mv : fast_aligned_mvs)
        {
            /* if ((value & mask) == pattern_value) */
#if defined(__AVX512F__)
            if (!_mm512_cmpeq_epi64_mask(
                  _mm512_and_si512(_mm512_loadu_si512(
                                     view_as<PatternByte::simd_value_t*>(
                                       current_data)),
                                   _mm512_load_si512(&mv.mask)),
                  _mm512_load_si512(&mv.value)))
            {
                goto skip;
            }
#elif defined(__AVX2__)
            if (_mm256_movemask_epi8(_mm256_cmpeq_epi64(
                  _mm256_and_si256(_mm256_loadu_si256(
                                     view_as<PatternByte::simd_value_t*>(
                                       current_data)),
                                   _mm256_load_si256(&mv.mask)),
                  _mm256_load_si256(&mv.value)))
                != -1)
            {
                goto skip;
            }
#else
            if ((*view_as<PatternByte::simd_value_t*>(current_data)
                 & mv.mask)
                != mv.value)
            {
                goto skip;
            }
#endif
            current_data += sizeof(PatternByte::simd_value_t);
        }

        matches.push_back(
          view_as<ptr_t>(view_as<uintptr_t>(baseAddress)
                         + view_as<uintptr_t>(start_data - data)));
    skip:
        start_data++;
        current_data = start_data;
    }

    return matches.size() != old_matches_size;
}

auto XKLib::PatternScanning::searchV2(XKLib::PatternByte& pattern,
                                      data_t data,
                                      std::size_t size,
                                      ptr_t baseAddress) -> bool
{
    auto&& pattern_bytes        = pattern.bytes();
    auto&& matches              = pattern.matches();
    auto old_matches_size       = matches.size();
    auto pattern_size           = pattern_bytes.size();
    auto&& vec_organized_values = pattern.vec_organized_values();

    data_t start_data   = data;
    data_t current_data = data;

    while (current_data + pattern_size <= data + size)
    {
        for (auto&& organized_values : vec_organized_values)
        {
            for (auto&& byte : organized_values.bytes)
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
          view_as<ptr_t>(view_as<uintptr_t>(baseAddress)
                         + view_as<uintptr_t>(start_data - data)));

    skip:
        start_data++;
        current_data = start_data;
    }

    return matches.size() != old_matches_size;
}

auto XKLib::PatternScanning::searchV3(XKLib::PatternByte& pattern,
                                      data_t data,
                                      std::size_t size,
                                      ptr_t baseAddress) -> bool
{
    auto&& matches          = pattern.matches();
    auto old_matches_size   = matches.size();
    auto&& pattern_bytes    = pattern.bytes();
    auto pattern_size       = pattern_bytes.size();
    auto&& fast_aligned_mvs = pattern.fast_aligned_mvs();

    data_t start_data   = data + size - pattern_size;
    data_t current_data = start_data;

    /**
     * Reversed Boyer Moore variant starts from the start of the pattern
     */
    auto it_mv = fast_aligned_mvs.begin();

    while (current_data >= data + pattern_size)
    {
        /**
         * Compare with our value (with unknown bytes, the mask)
         * Invert bits with cmp result and turn them into bits so we can
         * find the first set, so the mismatched byte.
         */
        const std::size_t mismatch_byte_num = __builtin_ffsll(
          ~_mm_cmp_pi8_simd_value(
            _mm_and_simd_value(_mm_loadu_simd_value(current_data),
                               _mm_load_simd_value(&it_mv->mask)),
            _mm_load_simd_value(&it_mv->value)));

        /* this part of the pattern mismatched ? */
        if (mismatch_byte_num > 0
            && mismatch_byte_num <= sizeof(PatternByte::simd_value_t))
        {
            /**
             * We got a mismatch, we need to re-align pattern / adjust
             * current data ptr
             */

            std::size_t to_skip = 0;

            /**
             * Do no enter if the last character of that part is
             * mismatching
             */
            if (mismatch_byte_num < it_mv->part_size)
            {
                /**
                 * Get the matched byte with a second mask,
                 * due to the fact that we've already
                 * checked previous values before entering here with the
                 * previous cmp
                 */

                constexpr auto bit_mask = []() constexpr
                {
                    static_assert(sizeof(PatternByte::simd_value_t) <= 64,
                                  "simd_value_t is bigger than 64 bytes");

                    if constexpr (sizeof(PatternByte::simd_value_t) == 64)
                    {
                        return std::numeric_limits<uint64_t>::max();
                    }
                    else if constexpr (sizeof(PatternByte::simd_value_t)
                                       < 64)
                    {
                        return (1ull << sizeof(PatternByte::simd_value_t))
                               - 1ull;
                    }
                }
                ();

                /* get the mismatched byte */
                const auto simd_tmp = _mm_set_pi8_simd_value(
                  *(current_data + mismatch_byte_num - 1));

                const std::size_t match_byte_num = __builtin_ffsll(
                  _mm_cmp_pi8_simd_value(
                    _mm_and_simd_value(simd_tmp,
                                       _mm_load_simd_value(&it_mv->mask)),
                    _mm_load_simd_value(&it_mv->value))
                  & (std::numeric_limits<uint64_t>::max()
                     << mismatch_byte_num)
                  & bit_mask);

                if (match_byte_num > 0
                    && match_byte_num <= it_mv->part_size)
                {
                    to_skip = match_byte_num - mismatch_byte_num;
                    goto good_char;
                }
                else
                {
                    to_skip = it_mv->part_size - mismatch_byte_num;
                }
            }

            /* pass on the next part */
            it_mv++;

            /* then search inside the rest of the pattern */
            if (it_mv != fast_aligned_mvs.end())
            {
                /* get the mismatched byte */
                const auto simd_tmp = _mm_set_pi8_simd_value(
                  *(current_data + mismatch_byte_num - 1));

                do
                {
                    const std::size_t match_byte_num = __builtin_ffsll(
                      _mm_cmp_pi8_simd_value(
                        _mm_and_simd_value(simd_tmp,
                                           _mm_load_simd_value(
                                             &it_mv->mask)),
                        _mm_load_simd_value(&it_mv->value)));

                    /**
                     * Matched, we found the mismatched byte inside
                     * the rest of the pattern
                     */
                    if (match_byte_num > 0
                        && match_byte_num <= it_mv->part_size)
                    {
                        to_skip += match_byte_num - 1;
                        /* exit */
                        goto good_char;
                    }

                    to_skip += it_mv->part_size;
                    it_mv++;
                }
                while (it_mv != fast_aligned_mvs.end());
            }

            /**
             * If bad, we skip past to the last mismatched char
             * set new cursor before the mismatched char (skip + 1)
             */
            to_skip++;

        good_char:
            /* otherwise just align pattern to the mismatch char */
            start_data -= to_skip;

            /* start from the beginning */
            it_mv = fast_aligned_mvs.begin();

            /* apply new cursor position */
            current_data = start_data;
        }
        else
        {
            /* did we found our stuff ? */
            if (it_mv == std::prev(fast_aligned_mvs.end()))
            {
                matches.push_back(view_as<ptr_t>(
                  view_as<uintptr_t>(baseAddress)
                  + view_as<uintptr_t>(start_data - data)));

                /* set new data cursor and reset pattern cursor */
                start_data -= pattern_size;
                current_data = start_data;
                it_mv        = fast_aligned_mvs.begin();
            }
            else
            {
                current_data += it_mv->part_size;
                it_mv++;
            }
        }
    }

    return matches.size() != old_matches_size;
}

auto XKLib::PatternScanning::searchV4(XKLib::PatternByte& pattern,
                                      data_t data,
                                      std::size_t size,
                                      ptr_t baseAddress) -> bool
{
    auto&& matches          = pattern.matches();
    auto old_matches_size   = matches.size();
    auto&& pattern_bytes    = pattern.bytes();
    auto pattern_size       = pattern_bytes.size();
    auto&& fast_aligned_mvs = pattern.fast_aligned_mvs();

    data_t start_data   = data + size - pattern_size;
    data_t current_data = start_data;

    /**
     * Reversed Boyer Moore variant starts from the start of the pattern
     */
    auto it_mv = fast_aligned_mvs.begin();

    while (current_data >= data + pattern_size)
    {
        /**
         * Compare with our value (with unknown bytes, the mask)
         * Invert bits with cmp result and turn them into bits so we can
         * find the first set, so the mismatched byte.
         */
        const std::size_t mismatch_byte_num = __builtin_ffsll(
          ~_mm_cmp_pi8_simd_value(
            _mm_and_simd_value(_mm_loadu_simd_value(current_data),
                               _mm_load_simd_value(&it_mv->mask)),
            _mm_load_simd_value(&it_mv->value)));

        /* this part of the pattern mismatched ? */
        if (mismatch_byte_num > 0
            && mismatch_byte_num <= sizeof(PatternByte::simd_value_t))
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
            it_mv = fast_aligned_mvs.begin();

            /* apply new cursor position */
            current_data = start_data;
        }
        else
        {
            /* did we found our stuff ? */
            if (it_mv == std::prev(fast_aligned_mvs.end()))
            {
                matches.push_back(view_as<ptr_t>(
                  view_as<uintptr_t>(baseAddress)
                  + view_as<uintptr_t>(start_data - data)));

                /* set new data cursor and reset pattern cursor */
                start_data -= pattern_size;
                current_data = start_data;
                it_mv        = fast_aligned_mvs.begin();
            }
            else
            {
                current_data += it_mv->part_size;
                it_mv++;
            }
        }
    }

    return matches.size() != old_matches_size;
}

auto XKLib::PatternScanning::searchAlignedV1(XKLib::PatternByte& pattern,
                                             data_t aligned_data,
                                             std::size_t size,
                                             ptr_t baseAddress) -> bool
{
    auto&& pattern_bytes    = pattern.bytes();
    auto&& matches          = pattern.matches();
    auto old_matches_size   = matches.size();
    auto pattern_size       = pattern_bytes.size();
    auto&& fast_aligned_mvs = pattern.fast_aligned_mvs();

    if ((view_as<uintptr_t>(aligned_data)
         % sizeof(PatternByte::simd_value_t))
        != 0)
    {
        XKLIB_EXCEPTION("Buffer is not aligned");
    }

    data_t start_data   = aligned_data;
    data_t current_data = aligned_data;

    /**
     * Here we are searching for a pattern that was aligned memory
     * on smid_value_t bits. This is really faster than the previous
     * methods.
     */
    while (current_data + pattern_size <= aligned_data + size)
    {
        for (auto&& mv : fast_aligned_mvs)
        {
/* if ((value & mask) == pattern_value) */
#if defined(__AVX512F__)
            if (!_mm512_cmpeq_epi64_mask(
                  _mm512_and_si512(_mm512_load_si512(
                                     view_as<PatternByte::simd_value_t*>(
                                       current_data)),
                                   _mm512_load_si512(&mv.mask)),
                  _mm512_load_si512(&mv.value)))
            {
                goto skip;
            }
#elif defined(__AVX2__)
            if (_mm256_movemask_epi8(_mm256_cmpeq_epi64(
                  _mm256_and_si256(_mm256_load_si256(
                                     view_as<PatternByte::simd_value_t*>(
                                       current_data)),
                                   _mm256_load_si256(&mv.mask)),
                  _mm256_load_si256(&mv.value)))
                != -1)
            {
                goto skip;
            }
#else
            if ((*view_as<PatternByte::simd_value_t*>(current_data)
                 & mv.mask)
                != mv.value)
            {
                goto skip;
            }
#endif
            current_data += sizeof(PatternByte::simd_value_t);
        }

        matches.push_back(view_as<ptr_t>(
          view_as<uintptr_t>(baseAddress)
          + view_as<uintptr_t>(start_data - aligned_data)));

    skip:
        start_data += sizeof(PatternByte::simd_value_t);
        current_data = start_data;
    }

    return matches.size() != old_matches_size;
}

auto XKLib::PatternScanning::searchInProcess(
  XKLib::PatternByte& pattern,
  Process& process,
  const std::function<
    auto(PatternByte&, data_t, std::size_t, ptr_t)->bool>& searchMethod)
  -> void
{
    auto mmap     = process.mmap();
    auto areaName = pattern.areaName();

    if (areaName.empty())
    {
        for (auto&& area : mmap.areas())
        {
            if (area->isReadable())
            {
                auto area_read = area->read<PatternByte::simd_value_t>();
                searchMethod(pattern,
                             view_as<data_t>(area_read.data()),
                             area->size(),
                             area->begin<ptr_t>());
            }
        }
    }
    else
    {
        searchInProcessWithAreaName(pattern, process, areaName);
    }
}

auto XKLib::PatternScanning::searchInProcessWithAreaName(
  XKLib::PatternByte& pattern,
  Process& process,
  const std::string& areaName,
  const std::function<
    auto(PatternByte&, data_t, std::size_t, ptr_t)->bool>& searchMethod)
  -> void
{
    auto mmap = process.mmap();

    for (auto&& area : mmap.areas())
    {
        if (area->isReadable()
            && (area->name().find(areaName) != std::string::npos))
        {
            auto area_read = area->read<PatternByte::simd_value_t>();
            searchMethod(pattern,
                         view_as<data_t>(area_read.data()),
                         area->size(),
                         area->begin<ptr_t>());
        }
    }
}
