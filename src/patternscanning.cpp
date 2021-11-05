#include "patternscanning.h"
#include "patternbyte.h"
#include <cstring>

auto XKLib::PatternScanning::searchV1(XKLib::PatternByte& pattern,
                                      data_t data,
                                      size_t size,
                                      ptr_t baseAddress) -> bool
{
    auto&& matches                 = pattern.matches();
    auto old_matches_size          = matches.size();
    auto&& pattern_values          = pattern.values();
    auto pattern_values_size       = pattern_values.size();
    auto fast_aligned_values       = pattern.fast_aligned_values();
    auto fast_aligned_masks        = pattern.fast_aligned_masks();
    auto fast_aligned_values_count = pattern.fast_aligned_values_count();

    size_t index            = 0;
    size_t start_index      = 0;
    size_t simd_value_index = 0;

    do
    {
        do
        {
#if defined(__AVX512F__)
            if (!_mm512_cmpeq_epi64_mask(
                  _mm512_and_si512(_mm512_loadu_si512(
                                     view_as<PatternByte::simd_value_t*>(
                                       &data[start_index])),
                                   fast_aligned_masks[simd_value_index]),
                  fast_aligned_values[simd_value_index]))
            {
                goto skip;
            }
#elif defined(__AVX2__)
            if (!_mm256_movemask_epi8(_mm256_cmpeq_epi64(
                  _mm256_and_si256(_mm256_loadu_si256(
                                     view_as<PatternByte::simd_value_t*>(
                                       &data[start_index])),
                                   fast_aligned_masks[simd_value_index]),
                  fast_aligned_values[simd_value_index])))
            {
                goto skip;
            }
#else
            if (fast_aligned_values[simd_value_index]
                != (*view_as<PatternByte::simd_value_t*>(
                      &data[start_index])
                    & fast_aligned_masks[simd_value_index]))
            {
                goto skip;
            }
#endif
            start_index += sizeof(PatternByte::simd_value_t);
            simd_value_index++;
        }
        while (simd_value_index < fast_aligned_values_count);

        matches.push_back(
          view_as<ptr_t>(view_as<uintptr_t>(baseAddress) + index));

    skip:
        index++;
        start_index      = index;
        simd_value_index = 0;
    }
    while (index + pattern_values_size <= size);

    return matches.size() != old_matches_size;
}

auto XKLib::PatternScanning::searchV2(XKLib::PatternByte& pattern,
                                      data_t data,
                                      size_t size,
                                      ptr_t baseAddress) -> bool
{
    auto&& pattern_values        = pattern.values();
    auto&& matches               = pattern.matches();
    auto old_matches_size        = matches.size();
    auto pattern_size            = pattern_values.size();
    auto&& vec_known_values      = pattern.vec_known_values();
    auto&& vec_skipper_uk_values = pattern.vec_skipper_uk_values();
    auto vec_known_values_size   = vec_known_values.size();
    size_t index_known_value     = 0;
    size_t start_index           = 0;
    size_t skipper_index         = 0;
    size_t index                 = 0;
    auto known_values            = vec_known_values[0];

    do
    {
        do
        {
            do
            {
                if (known_values[index_known_value] != data[start_index])
                {
                    index_known_value = 0;
                    known_values      = vec_known_values[0];
                    goto skip;
                }

                index_known_value++;
                start_index++;
            }
            while (index_known_value < known_values.size());

            start_index += vec_skipper_uk_values[skipper_index];
            skipper_index++;
            index_known_value = 0;

            if (skipper_index >= vec_known_values_size)
            {
                break;
            }

            known_values = vec_known_values[skipper_index];
        }
        while (true);

        matches.push_back(
          view_as<ptr_t>(view_as<uintptr_t>(baseAddress) + index));

    skip:
        index++;
        start_index   = index;
        skipper_index = 0;
    }
    while ((index + pattern_size) <= size);

    return matches.size() != old_matches_size;
}

auto XKLib::PatternScanning::searchAlignedV2(XKLib::PatternByte& pattern,
                                             data_t aligned_data,
                                             size_t size,
                                             ptr_t baseAddress) -> bool
{
    auto&& pattern_values          = pattern.values();
    auto&& matches                 = pattern.matches();
    auto old_matches_size          = matches.size();
    auto pattern_size              = pattern_values.size();
    auto fast_aligned_values       = pattern.fast_aligned_values();
    auto fast_aligned_masks        = pattern.fast_aligned_masks();
    auto fast_aligned_values_count = pattern.fast_aligned_values_count();

    size_t index            = 0;
    size_t simd_value_index = 0;
    size_t start_index      = 0;

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
        do
        {
#if defined(__AVX512F__)
            if (!_mm512_cmpeq_epi64_mask(
                  _mm512_and_si512(_mm512_load_si512(
                                     view_as<PatternByte::simd_value_t*>(
                                       &aligned_data[start_index])),
                                   fast_aligned_masks[simd_value_index]),
                  fast_aligned_values[simd_value_index]))
            {
                goto skip;
            }
#elif defined(__AVX2__)
            if (!_mm256_movemask_epi8(_mm256_cmpeq_epi64(
                  _mm256_and_si256(_mm256_load_si256(
                                     view_as<PatternByte::simd_value_t*>(
                                       &aligned_data[start_index])),
                                   fast_aligned_masks[simd_value_index]),
                  fast_aligned_values[simd_value_index])))
            {
                goto skip;
            }
#else
            if ((*view_as<PatternByte::simd_value_t*>(
                   &aligned_data[start_index])
                 & fast_aligned_masks[simd_value_index])
                != fast_aligned_values[simd_value_index])
            {
                goto skip;
            }
#endif
            start_index += sizeof(PatternByte::simd_value_t);
            simd_value_index++;
        }
        while (simd_value_index < fast_aligned_values_count);

        matches.push_back(
          view_as<ptr_t>(view_as<uintptr_t>(baseAddress) + index));

    skip:
        index += sizeof(PatternByte::simd_value_t);
        start_index      = index;
        simd_value_index = 0;
    }
    while ((index + pattern_size) <= size);

    return matches.size() != old_matches_size;
}

auto XKLib::PatternScanning::searchMethodThatOthersUsesAndItsBad(
  XKLib::PatternByte& pattern,
  data_t data,
  size_t size,
  ptr_t baseAddress) -> bool
{
    auto&& pattern_values     = pattern.values();
    auto&& matches            = pattern.matches();
    auto old_matches_size     = matches.size();
    auto pattern_size         = pattern_values.size();
    size_t index_pattern_byte = 1;
    size_t index              = 0;

    do
    {
        if (pattern_values[0]->value == data[index])
        {
            do
            {
                if (pattern_values[index_pattern_byte]->value
                    == PatternByte::Value::UNKNOWN)
                {
                    index_pattern_byte++;
                }
                else if (pattern_values[index_pattern_byte]->value
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
