#include "patternscanning.h"
#include "patternbyte.h"
#include <cstring>

auto XKLib::PatternScanning::searchV1(XKLib::PatternByte& pattern,
                                      data_t data,
                                      size_t size,
                                      ptr_t baseAddress) -> bool
{
    size_t simd_value_index;
    size_t start_index;
    auto&& pattern_values      = pattern.values();
    auto&& matches             = pattern.matches();
    auto old_matches_size      = matches.size();
    auto&& unknown_values      = pattern.unknown_values();
    auto&& simd_values         = pattern.simd_values();
    auto pattern_size          = pattern_values.size();
    auto left_bytes_to_analyze = pattern_size
                                 % sizeof(PatternByte::simd_value_t);
    auto simd_count = MemoryUtils::align(pattern_size,
                                         sizeof(PatternByte::simd_value_t))
                      / sizeof(PatternByte::simd_value_t);

    /** Build the mask for the last value if needed */
    if (left_bytes_to_analyze > 0)
    {
        PatternByte::simd_value_t last_pattern_value;
#if (defined(__AVX512F__) || defined(__AVX2__))                          \
  && defined(PATTERN_UNALIGNED_SIMD_V1_V2)
        auto bytes_mask = left_bytes_to_analyze;
    #ifdef __AVX512F__
        std::array<int64_t, 8> e {};
    #elif defined(__AVX2__)
        std::array<int64_t, 4> e {};
    #endif
        size_t ei = 0;

        if (bytes_mask >= sizeof(int64_t))
        {
            for (; ei < e.size() && bytes_mask >= sizeof(int64_t); ei++)
            {
                *view_as<uint64_t*>(&e[ei]) = std::numeric_limits<
                  uint64_t>::max();
                bytes_mask -= sizeof(int64_t);
            }

            if (bytes_mask > 0)
            {
                *view_as<uint64_t*>(&e[ei]) = (1ull
                                               << (bytes_mask * CHAR_BIT))
                                              - 1ull;
            }
        }
        else
        {
            *view_as<uint64_t*>(&e[ei]) = (1ull
                                           << (bytes_mask * CHAR_BIT))
                                          - 1ull;
        }
#endif

#if defined(__AVX512F__) && defined(PATTERN_UNALIGNED_SIMD_V1_V2)
        auto mask = _mm512_set_epi64(e[7],
                                     e[6],
                                     e[5],
                                     e[4],
                                     e[3],
                                     e[2],
                                     e[1],
                                     e[0]);
#elif defined(__AVX2__) && defined(PATTERN_UNALIGNED_SIMD_V1_V2)
        auto mask = _mm256_set_epi64x(e[3], e[2], e[1], e[0]);

#else
        PatternByte::simd_value_t mask = (1ull << left_bytes_to_analyze
                                                    * CHAR_BIT)
                                         - 1ull;

#endif
        for (size_t index = 0; (index + pattern_size) <= size; index++)
        {
            /**
             * Fill unknown bytes by the data we want to compare,
             * this permits to gain few instructions compared to a mask
             * every iterations our simd value loaded.
             */
            for (auto&& unknown_value : unknown_values)
            {
                std::memcpy(&view_as<data_t>(
                              &simd_values[unknown_value.simd_index])
                              [unknown_value.simd_byte_index],
                            &data[index + unknown_value.data_byte_index],
                            unknown_value.size_to_copy);
            }

#if (defined(__AVX512F__) || defined(__AVX2__))                          \
  && defined(PATTERN_UNALIGNED_SIMD_V1_V2)
            /**
             * See if the memory is aligned, if yes we can load much
             * faster values.
             */
            auto is_aligned = view_as<size_t>(&data[index])
                                  % sizeof(PatternByte::simd_value_t) ?
                                false :
                                true;
#endif

#if defined(__AVX512F__) && defined(PATTERN_UNALIGNED_SIMD_V1_V2)
            last_pattern_value = _mm512_and_si512(simd_values[simd_count],
                                                  mask);
#elif defined(__AVX2__) && defined(PATTERN_UNALIGNED_SIMD_V1_V2)
            last_pattern_value = _mm256_and_si256(simd_values[simd_count],
                                                  mask);
#else
            last_pattern_value = simd_values[simd_count] & mask;
#endif

            start_index = index;

            for (simd_value_index = 0; simd_value_index < simd_count;
                 simd_value_index++)
            {
#if defined(__AVX512F__) && defined(PATTERN_UNALIGNED_SIMD_V1_V2)
                if (!_mm512_cmpeq_epi64_mask(
                      is_aligned ? _mm512_load_si512(
                        view_as<PatternByte::simd_value_t*>(
                          &data[start_index])) :
                                   _mm512_loadu_si512(
                                     view_as<PatternByte::simd_value_t*>(
                                       &data[start_index])),
                      simd_values[simd_value_index]))
                {
                    goto skip;
                }
#elif defined(__AVX2__) && defined(PATTERN_UNALIGNED_SIMD_V1_V2)
                if (!_mm256_movemask_epi8(_mm256_cmpeq_epi64(
                      is_aligned ? _mm256_load_si256(
                        view_as<PatternByte::simd_value_t*>(
                          &data[start_index])) :
                                   _mm256_loadu_si256(
                                     view_as<PatternByte::simd_value_t*>(
                                       &data[start_index])),
                      simd_values[simd_value_index])))
                {
                    goto skip;
                }
#else
                if (*view_as<PatternByte::simd_value_t*>(
                      &data[start_index])
                    != simd_values[simd_value_index])
                {
                    goto skip;
                }
#endif
                start_index += sizeof(PatternByte::simd_value_t);
            }

#if defined(__AVX512F__) && defined(PATTERN_UNALIGNED_SIMD_V1_V2)
            if (!_mm512_cmpeq_epi64_mask(
                  _mm512_and_si512(
                    is_aligned ? _mm512_load_si512(
                      view_as<PatternByte::simd_value_t*>(
                        &data[start_index])) :
                                 _mm512_loadu_si512(
                                   view_as<PatternByte::simd_value_t*>(
                                     &data[start_index])),
                    mask),
                  last_pattern_value))
            {
                goto skip;
            }
#elif defined(__AVX2__) && defined(PATTERN_UNALIGNED_SIMD_V1_V2)
            if (!_mm256_movemask_epi8(_mm256_cmpeq_epi64(
                  _mm256_and_si256(
                    is_aligned ? _mm256_load_si256(
                      view_as<PatternByte::simd_value_t*>(
                        &data[start_index])) :
                                 _mm256_loadu_si256(
                                   view_as<PatternByte::simd_value_t*>(
                                     &data[start_index])),
                    mask),
                  last_pattern_value)))
            {
                goto skip;
            }
#else
            if ((*view_as<PatternByte::simd_value_t*>(&data[start_index])
                 & mask)
                != last_pattern_value)
            {
                goto skip;
            }
#endif

            pattern.matches().push_back(
              view_as<ptr_t>(view_as<uintptr_t>(baseAddress) + index));

        skip:;
        }
    }
    else
    {
        for (size_t index = 0; (index + pattern_size) <= size; index++)
        {
            /**
             * Fill unknown bytes by the data we want to compare,
             * this permits to gain few instructions compared to a mask
             * every iterations our simd value loaded.
             */
            for (auto&& unknown_value : unknown_values)
            {
                std::memcpy(&view_as<data_t>(
                              &simd_values[unknown_value.simd_index])
                              [unknown_value.simd_byte_index],
                            &data[index + unknown_value.data_byte_index],
                            unknown_value.size_to_copy);
            }

#if (defined(__AVX512F__) || defined(__AVX2__))                          \
  && defined(PATTERN_UNALIGNED_SIMD_V1_V2)
            /**
             * See if the memory is aligned, if yes we can load much
             * faster values.
             */
            auto is_aligned = view_as<size_t>(&data[index])
                                  % sizeof(PatternByte::simd_value_t) ?
                                false :
                                true;
#endif
            start_index = index;

            for (simd_value_index = 0; simd_value_index < simd_count;
                 simd_value_index++)
            {
#if defined(__AVX512F__) && defined(PATTERN_UNALIGNED_SIMD_V1_V2)
                if (!_mm512_cmpeq_epi64_mask(
                      is_aligned ? _mm512_load_si512(
                        view_as<PatternByte::simd_value_t*>(
                          &data[start_index])) :
                                   _mm512_loadu_si512(
                                     view_as<PatternByte::simd_value_t*>(
                                       &data[start_index])),
                      simd_values[simd_value_index]))
                {
                    goto skip2;
                }
#elif defined(__AVX2__) && defined(PATTERN_UNALIGNED_SIMD_V1_V2)
                if (!_mm256_movemask_epi8(_mm256_cmpeq_epi64(
                      is_aligned ? _mm256_load_si256(
                        view_as<PatternByte::simd_value_t*>(
                          &data[start_index])) :
                                   _mm256_loadu_si256(
                                     view_as<PatternByte::simd_value_t*>(
                                       &data[start_index])),
                      simd_values[simd_value_index])))
                {
                    goto skip2;
                }
#else
                if (*view_as<PatternByte::simd_value_t*>(
                      &data[start_index])
                    != simd_values[simd_value_index])
                {
                    goto skip2;
                }
#endif
                start_index += sizeof(PatternByte::simd_value_t);
            }

            pattern.matches().push_back(
              view_as<ptr_t>(view_as<uintptr_t>(baseAddress) + index));

        skip2:;
        }
    }

    return matches.size() != old_matches_size;
}

auto XKLib::PatternScanning::searchV2(XKLib::PatternByte& pattern,
                                      data_t data,
                                      size_t size,
                                      ptr_t baseAddress) -> bool
{
    auto&& pattern_fvalues = pattern.fvalues();
    auto old_matches_size  = pattern.matches().size();
    auto&& pattern_values  = pattern.values();

    for (size_t index = 0; (index + pattern_values.size()) <= size;
         index++)
    {
        auto start_index = index;

        for (auto&& pattern_value : pattern_fvalues)
        {
#if (defined(__AVX512F__) || defined(__AVX2__))                          \
  && defined(PATTERN_UNALIGNED_SIMD_V1_V2)
            auto is_aligned = view_as<size_t>(&data[start_index])
                                  % sizeof(PatternByte::simd_value_t) ?
                                false :
                                true;
#endif

#if defined(__AVX512F__) && defined(PATTERN_UNALIGNED_SIMD_V1_V2)
            if (!pattern_value.unknown
                && !_mm512_cmpeq_epi64_mask(
                  _mm512_and_si512(
                    is_aligned ? _mm512_load_si512(
                      view_as<PatternByte::simd_value_t*>(
                        &data[start_index])) :
                                 _mm512_loadu_si512(
                                   view_as<PatternByte::simd_value_t*>(
                                     &data[start_index])),
                    pattern_value.mask),
                  pattern_value.value))
            {
                goto skip;
            }
#elif defined(__AVX2__) && defined(PATTERN_UNALIGNED_SIMD_V1_V2)
            if (!pattern_value.unknown
                && !_mm256_movemask_epi8(_mm256_cmpeq_epi64(
                  _mm256_and_si256(
                    is_aligned ? _mm256_load_si256(
                      view_as<PatternByte::simd_value_t*>(
                        &data[start_index])) :
                                 _mm256_loadu_si256(
                                   view_as<PatternByte::simd_value_t*>(
                                     &data[start_index])),
                    pattern_value.mask),
                  pattern_value.value)))
            {
                goto skip;
            }
#else
            if (!pattern_value.unknown
                && pattern_value.value
                     != (*view_as<PatternByte::simd_value_t*>(
                           &data[start_index])
                         & pattern_value.mask))
            {
                goto skip;
            }
#endif
            start_index += pattern_value.var_size;
        }

        pattern.matches().push_back(
          view_as<ptr_t>(view_as<uintptr_t>(baseAddress) + index));

    skip:;
    }

    return pattern.matches().size() != old_matches_size;
}

auto XKLib::PatternScanning::searchV3(XKLib::PatternByte& pattern,
                                      data_t data,
                                      size_t size,
                                      ptr_t baseAddress) -> bool
{
    auto& pattern_values  = pattern.values();
    auto old_matches_size = pattern.matches().size();
    auto pattern_size     = pattern_values.size();
    size_t index_pattern_byte;
    size_t start_index;

    for (size_t index = 0; (index + pattern_size) <= size; index++)
    {
        /**
         * TODO:
         * This would need to be more generic !!!
         * Basically we would need to compare the first known values,
         * so we can avoid the unknown byte check properly.
         */
        if (pattern_values[0]->value == data[index])
        {
            start_index = index;

            for (index_pattern_byte = 1;
                 index_pattern_byte < pattern_size;
                 index_pattern_byte++)
            {
                if (pattern_values[index_pattern_byte]->value
                    == PatternByte::Value::UNKNOWN)
                {
                    start_index++;
                }
                else if (pattern_values[index_pattern_byte]->value
                         != data[start_index])
                {
                    goto skip;
                }
            }

            pattern.matches().push_back(
              view_as<ptr_t>(view_as<uintptr_t>(baseAddress) + index));
        }
    skip:;
    }

    return pattern.matches().size() != old_matches_size;
}

auto XKLib::PatternScanning::searchAlignedV1(XKLib::PatternByte& pattern,
                                             data_t aligned_data,
                                             size_t size,
                                             ptr_t baseAddress) -> bool
{
    size_t simd_value_index;
    size_t start_index;
    auto&& pattern_values = pattern.values();
    auto&& matches        = pattern.matches();
    auto old_matches_size = matches.size();
    auto&& unknown_values = pattern.unknown_values();
    auto&& simd_values    = pattern.simd_values();
    auto pattern_size     = pattern_values.size();
    auto simd_count       = MemoryUtils::align(pattern_size,
                                         sizeof(PatternByte::simd_value_t))
                      / sizeof(PatternByte::simd_value_t);

    if ((view_as<uintptr_t>(aligned_data)
         % sizeof(PatternByte::simd_value_t))
        != 0)
    {
        XKLIB_EXCEPTION("Buffer is not aligned");
    }

    /**
     * Here we are searching for a pattern that was aligned memory
     * on smid_value_t bits. This is really faster than the previous
     * methods.
     */
    for (size_t index = 0; (index + pattern_size) <= size;
         index += sizeof(PatternByte::simd_value_t))
    {
        /**
         * Fill unknown bytes by the data we want to compare,
         * this permits to gain few instructions compared to a mask
         * every iterations our simd value loaded.
         */
        for (auto&& unknown_value : unknown_values)
        {
            std::memcpy(
              &view_as<data_t>(&simd_values[unknown_value.simd_index])
                [unknown_value.simd_byte_index],
              &aligned_data[index + unknown_value.data_byte_index],
              unknown_value.size_to_copy);
        }

        start_index = index;

        for (simd_value_index = 0; simd_value_index < simd_count;
             simd_value_index++)
        {
#if defined(__AVX512F__)
            if (!_mm512_cmpeq_epi64_mask(
                  _mm512_load_si512(view_as<PatternByte::simd_value_t*>(
                    &aligned_data[start_index])),
                  simd_values[simd_value_index]))
            {
                goto skip;
            }
#elif defined(__AVX2__)
            if (!_mm256_movemask_epi8(_mm256_cmpeq_epi64(
                  _mm256_load_si256(view_as<PatternByte::simd_value_t*>(
                    &aligned_data[start_index])),
                  simd_values[simd_value_index])))
            {
                goto skip;
            }
#else
            if (*view_as<PatternByte::simd_value_t*>(
                  &aligned_data[start_index])
                != simd_values[simd_value_index])
            {
                goto skip;
            }
#endif
            start_index += sizeof(PatternByte::simd_value_t);
        }

        pattern.matches().push_back(
          view_as<ptr_t>(view_as<uintptr_t>(baseAddress) + index));

    skip:;
    }

    return pattern.matches().size() != old_matches_size;
}

auto XKLib::PatternScanning::searchAlignedV2(XKLib::PatternByte& pattern,
                                             data_t aligned_data,
                                             size_t size,
                                             ptr_t baseAddress) -> bool
{
    size_t simd_value_index;
    size_t start_index;
    auto&& pattern_values = pattern.values();
    auto&& matches        = pattern.matches();
    auto old_matches_size = matches.size();
    auto&& simd_values    = pattern.simd_values();
    auto pattern_size     = pattern_values.size();
    auto simd_count       = MemoryUtils::align(pattern_size,
                                         sizeof(PatternByte::simd_value_t))
                      / sizeof(PatternByte::simd_value_t);
    auto&& fast_masks = pattern.fast_masks();

    if ((view_as<uintptr_t>(aligned_data)
         % sizeof(PatternByte::simd_value_t))
        != 0)
    {
        XKLIB_EXCEPTION("Buffer is not aligned");
    }

    /**
     * Here we are searching for a pattern that was aligned memory
     * on smid_value_t bits. This is really faster than the previous
     * methods.
     */
    for (size_t index = 0; (index + pattern_size) <= size;
         index += sizeof(PatternByte::simd_value_t))
    {
        start_index = index;

        for (simd_value_index = 0; simd_value_index < simd_count;
             simd_value_index++)
        {
#if defined(__AVX512F__)
            if (!_mm512_cmpeq_epi64_mask(
                  _mm512_and_si512(_mm512_load_si512(
                                     view_as<PatternByte::simd_value_t*>(
                                       &aligned_data[start_index])),
                                   fast_masks[simd_value_index]),
                  simd_values[simd_value_index]))
            {
                goto skip;
            }
#elif defined(__AVX2__)
            if (!_mm256_movemask_epi8(_mm256_cmpeq_epi64(
                  _mm256_and_si256(_mm256_load_si256(
                                     view_as<PatternByte::simd_value_t*>(
                                       &aligned_data[start_index])),
                                   fast_masks[simd_value_index]),
                  simd_values[simd_value_index])))
            {
                goto skip;
            }
#else
            if ((*view_as<PatternByte::simd_value_t*>(
                   &aligned_data[start_index])
                 & fast_masks[simd_value_index])
                != simd_values[simd_value_index])
            {
                goto skip;
            }
#endif
            start_index += sizeof(PatternByte::simd_value_t);
        }

        pattern.matches().push_back(
          view_as<ptr_t>(view_as<uintptr_t>(baseAddress) + index));

    skip:;
    }

    return pattern.matches().size() != old_matches_size;
}

auto XKLib::PatternScanning::searchInProcess(
  XKLib::PatternByte& pattern,
  Process& process,
  const std::function<auto(PatternByte&, data_t, size_t, ptr_t)->bool>&
    searchMethod) -> void
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
  const std::function<auto(PatternByte&, data_t, size_t, ptr_t)->bool>&
    searchMethod) -> void
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
