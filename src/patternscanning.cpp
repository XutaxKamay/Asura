#include "patternscanning.h"
#include "patternbyte.h"
#include <cstring>

auto XKLib::PatternScanning::search(XKLib::PatternByte& pattern,
                                    const XKLib::bytes_t& bytes,
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

    auto buffer_size = bytes.size();

    /** Build the mask for the last value if needed */
    if (left_bytes_to_analyze > 0)
    {
        PatternByte::simd_value_t last_pattern_value;
#if defined(__AVX512F__) || defined(__AVX2__)
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

#ifdef __AVX512F__
        auto mask = _mm512_set_epi64(e[7],
                                     e[6],
                                     e[5],
                                     e[4],
                                     e[3],
                                     e[2],
                                     e[1],
                                     e[0]);
#elif defined(__AVX2__)
        auto mask = _mm256_set_epi64x(e[3], e[2], e[1], e[0]);

#else
        PatternByte::simd_value_t mask = (1ull << left_bytes_to_analyze
                                                    * CHAR_BIT)
                                         - 1ull;

#endif
        for (size_t index = 0; (index + pattern_size) <= buffer_size;
             index++)
        {
            /**
             * Fill unknown bytes by the data we want to compare,
             * this permits to gain few cycles compared to a mask every
             * iterations our simd value loaded.
             */
            for (auto&& unknown_value : unknown_values)
            {
                std::memcpy(&view_as<data_t>(
                              &simd_values[unknown_value.simd_index])
                              [unknown_value.simd_byte_index],
                            &bytes[index + unknown_value.data_byte_index],
                            unknown_value.size_to_copy);
            }

#if defined(__AVX512F__) || defined(__AVX2__)
            /**
             * See if the memory is aligned, if yes we can load much
             * faster values.
             */
            auto is_aligned = view_as<size_t>(&bytes[index])
                                  % sizeof(PatternByte::simd_value_t) ?
                                true :
                                false;
#endif

#if defined(__AVX512F__)
            last_pattern_value = _mm512_and_si512(simd_values[simd_count],
                                                  mask);
#elif defined(__AVX2__)
            last_pattern_value = _mm256_and_si256(simd_values[simd_count],
                                                  mask);
#else
            last_pattern_value = simd_values[simd_count] & mask;
#endif

            start_index = index;

            for (simd_value_index = 0; simd_value_index < simd_count;
                 simd_value_index++)
            {
#if defined(__AVX512F__)
                if (!_mm512_cmpeq_epi64_mask(
                      is_aligned ? _mm512_load_si512(
                        view_as<PatternByte::simd_value_t*>(
                          &bytes[start_index])) :
                                   _mm512_loadu_si512(
                                     view_as<PatternByte::simd_value_t*>(
                                       &bytes[start_index])),
                      simd_values[simd_value_index]))
                {
                    goto skip;
                }
#elif defined(__AVX2__)
                if (!_mm256_movemask_epi8(_mm256_cmpeq_epi64(
                      is_aligned ? _mm256_load_si256(
                        view_as<PatternByte::simd_value_t*>(
                          &bytes[start_index])) :
                                   _mm256_loadu_si256(
                                     view_as<PatternByte::simd_value_t*>(
                                       &bytes[start_index])),
                      simd_values[simd_value_index])))
                {
                    goto skip;
                }
#else
                if (*view_as<PatternByte::simd_value_t*>(
                      &bytes[start_index])
                    != simd_values[simd_value_index])
                {
                    goto skip;
                }
#endif
                start_index += sizeof(PatternByte::simd_value_t);
            }

#if defined(__AVX512F__)
            if (!_mm512_cmpeq_epi64_mask(
                  _mm512_and_si512(
                    is_aligned ? _mm512_load_si512(
                      view_as<PatternByte::simd_value_t*>(
                        &bytes[start_index])) :
                                 _mm512_loadu_si512(
                                   view_as<PatternByte::simd_value_t*>(
                                     &bytes[start_index])),
                    mask),
                  last_pattern_value))
            {
                goto skip;
            }
#elif defined(__AVX2__)
            if (!_mm256_movemask_epi8(_mm256_cmpeq_epi64(
                  _mm256_and_si256(
                    is_aligned ? _mm256_load_si256(
                      view_as<PatternByte::simd_value_t*>(
                        &bytes[start_index])) :
                                 _mm256_loadu_si256(
                                   view_as<PatternByte::simd_value_t*>(
                                     &bytes[start_index])),
                    mask),
                  last_pattern_value)))
            {
                goto skip;
            }
#else
            if ((*view_as<PatternByte::simd_value_t*>(&bytes[start_index])
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
        for (size_t index = 0; (index + pattern_size) <= buffer_size;
             index++)
        {
            /**
             * Fill unknown bytes by the data we want to compare,
             * this permits to gain few cycles compared to a mask every
             * iterations our simd value loaded.
             */
            for (auto&& unknown_value : unknown_values)
            {
                std::memcpy(&view_as<data_t>(
                              &simd_values[unknown_value.simd_index])
                              [unknown_value.simd_byte_index],
                            &bytes[index + unknown_value.data_byte_index],
                            unknown_value.size_to_copy);
            }

#if defined(__AVX512F__) || defined(__AVX2__)
            /**
             * See if the memory is aligned, if yes we can load much
             * faster values.
             */
            auto is_aligned = view_as<size_t>(&bytes[index])
                                  % sizeof(PatternByte::simd_value_t) ?
                                true :
                                false;
#endif
            start_index = index;

            for (simd_value_index = 0; simd_value_index < simd_count;
                 simd_value_index++)
            {
#if defined(__AVX512F__)
                if (!_mm512_cmpeq_epi64_mask(
                      is_aligned ? _mm512_load_si512(
                        view_as<PatternByte::simd_value_t*>(
                          &bytes[start_index])) :
                                   _mm512_loadu_si512(
                                     view_as<PatternByte::simd_value_t*>(
                                       &bytes[start_index])),
                      simd_values[simd_value_index]))
                {
                    goto skip2;
                }
#elif defined(__AVX2__)
                if (!_mm256_movemask_epi8(_mm256_cmpeq_epi64(
                      is_aligned ? _mm256_load_si256(
                        view_as<PatternByte::simd_value_t*>(
                          &bytes[start_index])) :
                                   _mm256_loadu_si256(
                                     view_as<PatternByte::simd_value_t*>(
                                       &bytes[start_index])),
                      simd_values[simd_value_index])))
                {
                    goto skip2;
                }
#else
                if (*view_as<PatternByte::simd_value_t*>(
                      &bytes[start_index])
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

// auto XKLib::PatternScanning::search(XKLib::PatternByte& pattern,
//                                    XKLib::const bytes_t& bytes,
//                                    ptr_t baseAddress) -> bool
//{
//    auto& pattern_values  = pattern.values();
//    auto old_matches_size = pattern.matches().size();
//    auto buffer_size      = bytes.size();

//    for (size_t index = 0; index < buffer_size; index++)
//    {
//       if ((start_index + pattern_values.size()) > buffer_size)
//       {
//          return pattern.matches().size() != old_matches_size;
//       }

//        /* Then scan the rest */
//        auto start_index = index;

//        for (auto&& pattern_byte : pattern_values)
//        {
//            if (pattern_byte.value !=
//            PatternByte::Value::type_t::UNKNOWN
//                && pattern_byte.value != bytes[start_index])
//            {
//                goto skip;
//            }

//            start_index++;
//        }

//        pattern.matches().push_back(
//          view_as<ptr_t>(view_as<uintptr_t>(baseAddress) + index));

//    skip:;
//    }

//    return pattern.matches().size() != old_matches_size;
//}

auto XKLib::PatternScanning::searchInProcess(XKLib::PatternByte& pattern,
                                             Process& process) -> void
{
    auto mmap     = process.mmap();
    auto areaName = pattern.areaName();

    if (areaName.empty())
    {
        for (auto&& area : mmap.areas())
        {
            if (area->isReadable())
            {
                search(pattern, area->read(), area->begin<ptr_t>());
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
  const std::string& areaName) -> void
{
    auto mmap = process.mmap();

    for (auto&& area : mmap.areas())
    {
        if (area->isReadable()
            && (area->name().find(areaName) != std::string::npos))
        {
            search(pattern, area->read(), area->begin<ptr_t>());
        }
    }
}
