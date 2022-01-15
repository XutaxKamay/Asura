#include "pch.h"

#include "patternscanning.h"

#include "patternbyte.h"

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
        /* get masked value */
        const auto mask_value = _mm_and_si64(
          *view_as<__m64*>(current_data),
          *view_as<__m64*>(&it_mv->mask));

        /* compare with our value (with unknown bytes due to mask) */
        const auto cmp = _mm_cmpeq_pi8(mask_value,
                                       *view_as<__m64*>(&it_mv->value));

        /**
         * Invert bits with cmp result and turn them into bits so we can
         * find the first set, so the mismatched byte.
         */
        std::size_t mismatch_byte_num = __builtin_ffs(
          ~_mm_movemask_pi8(cmp));

        /* this part of the pattern mismatched ? */
        if (mismatch_byte_num > 0
            && mismatch_byte_num <= sizeof(PatternByte::simd_value_t))
        {
            /**
             * We got a mismatch, we need to re-align pattern / adjust
             * current data ptr
             */

            const auto index = mismatch_byte_num;

            /* Convert to index */
            mismatch_byte_num--;

            /* get the mismatched byte */
            const auto byte_mismatched = *(current_data
                                           + mismatch_byte_num);

            bool badChar        = true;
            std::size_t to_skip = 0;

            const auto simd_tmp = _mm_set1_pi8(byte_mismatched);

            /**
             * Do no enter if the last character of that part is
             * mismatching
             */
            if (index < it_mv->size)
            {
                const auto mask_value = _mm_and_si64(simd_tmp,
                                                     *view_as<__m64*>(
                                                       &it_mv->mask));

                /* 0xFF.. >> index * 8 */
                const auto other_mask = _mm_slli_si64(_mm_set1_pi8(0xFF),
                                                      index * CHAR_BIT);

                const auto cmp = _mm_and_si64(
                  _mm_cmpeq_pi8(mask_value,
                                *view_as<__m64*>(&it_mv->value)),
                  other_mask);

                const std::size_t match_byte_num = __builtin_ffs(
                  _mm_movemask_pi8(cmp));

                if (match_byte_num > 0 && match_byte_num <= it_mv->size)
                {
                    to_skip = match_byte_num - index;
                    badChar = false;
                }
                else
                {
                    to_skip = it_mv->size - index;
                }
            }

            /* Did we not find it ? */
            if (badChar)
            {
                /* Pass on the next part */
                it_mv++;

                /* Then search inside the rest of the pattern */
                while (it_mv != fast_aligned_mvs.end())
                {
                    const auto mask_value = _mm_and_si64(simd_tmp,
                                                         *view_as<__m64*>(
                                                           &it_mv->mask));

                    const auto cmp = _mm_cmpeq_pi8(mask_value,
                                                   *view_as<__m64*>(
                                                     &it_mv->value));

                    const std::size_t match_byte_num = __builtin_ffs(
                      _mm_movemask_pi8(cmp));

                    /**
                     * Matched, we found the mismatched byte inside
                     * the rest of the pattern
                     */
                    if (match_byte_num > 0
                        && match_byte_num <= it_mv->size)
                    {
                        to_skip += match_byte_num - 1;
                        badChar = false;
                        /* exit */
                        break;
                    }

                    to_skip += it_mv->size;
                    it_mv++;
                }
            }

            /* if bad, we skip past to the last mismatched char */
            if (badChar)
            {
                /* set new cursor before the mismatched char (skip + 1) */
                start_data -= to_skip + 1;
            }
            else
            {
                /* otherwise just align pattern to the mismatch char */
                start_data -= to_skip;
            }

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
                current_data += it_mv->size;
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
