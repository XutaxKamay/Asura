#include "pch.h"

#include "patternscanning.h"

#include "patternbyte.h"

auto XKLib::PatternScanning::searchV1(XKLib::PatternByte& pattern,
                                      data_t data,
                                      size_t size,
                                      ptr_t baseAddress) -> bool
{
    /* prepare stuffs */
    auto&& matches          = pattern.matches();
    auto old_matches_size   = matches.size();
    auto&& pattern_bytes    = pattern.bytes();
    auto pattern_bytes_size = pattern_bytes.size();
    auto&& fast_aligned_mvs = pattern.fast_aligned_mvs();

    size_t index         = 0;
    size_t current_index = 0;

    do
    {
        for (auto&& mv : fast_aligned_mvs)
        {
            /* if ((value & mask) == pattern_value) */
#if defined(__AVX512F__)
            if (!_mm512_cmpeq_epi64_mask(
                  _mm512_and_si512(_mm512_loadu_si512(
                                     view_as<PatternByte::simd_value_t*>(
                                       &data[current_index])),
                                   _mm512_load_si512(&mv.mask)),
                  _mm512_load_si512(&mv.value)))
            {
                goto skip;
            }
#elif defined(__AVX2__)
            if (_mm256_movemask_epi8(_mm256_cmpeq_epi64(
                  _mm256_and_si256(_mm256_loadu_si256(
                                     view_as<PatternByte::simd_value_t*>(
                                       &data[current_index])),
                                   _mm256_load_si256(&mv.mask)),
                  _mm256_load_si256(&mv.value)))
                != -1)
            {
                goto skip;
            }
#else
            if ((*view_as<PatternByte::simd_value_t*>(&data[current_index])
                 & mv.mask)
                != mv.value)
            {
                goto skip;
            }
#endif
            current_index += sizeof(PatternByte::simd_value_t);
        }

        matches.push_back(
          view_as<ptr_t>(view_as<uintptr_t>(baseAddress) + index));

    skip:
        index++;
        current_index = index;
    }
    while (index + pattern_bytes_size <= size);

    return matches.size() != old_matches_size;
}

auto XKLib::PatternScanning::searchV2(XKLib::PatternByte& pattern,
                                      data_t data,
                                      size_t size,
                                      ptr_t baseAddress) -> bool
{
    auto&& pattern_bytes        = pattern.bytes();
    auto&& matches              = pattern.matches();
    auto old_matches_size       = matches.size();
    auto pattern_size           = pattern_bytes.size();
    auto&& vec_organized_values = pattern.vec_organized_values();

    data_t start_data   = data;
    data_t current_data = data;

    do
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
    while ((start_data + pattern_size) <= (data + size));

    return matches.size() != old_matches_size;
}

auto XKLib::PatternScanning::searchAlignedV2(XKLib::PatternByte& pattern,
                                             data_t aligned_data,
                                             size_t size,
                                             ptr_t baseAddress) -> bool
{
    auto&& pattern_bytes    = pattern.bytes();
    auto&& matches          = pattern.matches();
    auto old_matches_size   = matches.size();
    auto pattern_size       = pattern_bytes.size();
    auto&& fast_aligned_mvs = pattern.fast_aligned_mvs();

    size_t index         = 0;
    size_t current_index = 0;

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
    do
    {
        for (auto&& mv : fast_aligned_mvs)
        {
/* if ((value & mask) == pattern_value) */
#if defined(__AVX512F__)
            if (!_mm512_cmpeq_epi64_mask(
                  _mm512_and_si512(_mm512_load_si512(
                                     view_as<PatternByte::simd_value_t*>(
                                       &aligned_data[current_index])),
                                   _mm512_load_si512(&mv.mask)),
                  _mm512_load_si512(&mv.value)))
            {
                goto skip;
            }
#elif defined(__AVX2__)
            if (_mm256_movemask_epi8(_mm256_cmpeq_epi64(
                  _mm256_and_si256(_mm256_load_si256(
                                     view_as<PatternByte::simd_value_t*>(
                                       &aligned_data[current_index])),
                                   _mm256_load_si256(&mv.mask)),
                  _mm256_load_si256(&mv.value)))
                != -1)
            {
                goto skip;
            }
#else
            if ((*view_as<PatternByte::simd_value_t*>(
                   &aligned_data[current_index])
                 & mv.mask)
                != mv.value)
            {
                goto skip;
            }
#endif
            current_index += sizeof(PatternByte::simd_value_t);
        }

        matches.push_back(
          view_as<ptr_t>(view_as<uintptr_t>(baseAddress) + index));

    skip:
        index += sizeof(PatternByte::simd_value_t);
        current_index = index;
    }
    while ((index + pattern_size) <= size);

    return matches.size() != old_matches_size;
}

auto XKLib::PatternScanning::searchTest(XKLib::PatternByte& pattern,
                                        data_t data,
                                        size_t size,
                                        ptr_t baseAddress) -> bool
{
    auto&& pattern_bytes  = pattern.bytes();
    auto&& matches        = pattern.matches();
    auto old_matches_size = matches.size();
    auto pattern_size     = pattern_bytes.size();

    size_t index              = 0;
    size_t index_pattern_byte = 1;

    do
    {
        if (pattern_bytes[0].value == data[index])
        {
            do
            {
                if (pattern_bytes[index_pattern_byte].value
                    == PatternByte::Value::UNKNOWN)
                {
                    index_pattern_byte++;
                }
                else if (pattern_bytes[index_pattern_byte].value
                         != data[index + index_pattern_byte])
                {
                    index_pattern_byte = 1;
                    goto skip;
                }
                else
                {
                    index_pattern_byte++;
                }
            }
            while (index_pattern_byte < pattern_size);

            matches.push_back(
              view_as<ptr_t>(view_as<uintptr_t>(baseAddress) + index));
        }
    skip:
        index++;
    }
    while ((index + pattern_size) <= size);

    return matches.size() != old_matches_size;
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
