#include "patternscanning.h"
#include "patternbyte.h"
#include <cstring>

auto XKLib::PatternScanning::search(XKLib::PatternByte& pattern,
                                    const XKLib::bytes_t& bytes,
                                    ptr_t baseAddress) -> bool
{
    auto&& matches        = pattern.matches();
    auto old_matches_size = matches.size();
    auto&& unknown_values = pattern.unknown_values();
    auto&& simd_values    = pattern.simd_values();
    auto&& pattern_values = pattern.values();

    for (size_t index = 0;
         index < bytes.size()
         && (index + pattern_values.size()) <= bytes.size();
         index++)
    {
        /* Fill unknown bytes by the data we want to compare */
        for (auto&& unknown_value : unknown_values)
        {
            auto simd_bytes = view_as<data_t>(
              &simd_values[unknown_value.simd_index]);
            simd_bytes[unknown_value.byte_index] = bytes
              [index + unknown_value.index];
        }

        auto left = view_as<size_t>(&bytes[index])
                    % sizeof(PatternByte::simd_value_t);

        auto start_index = index;

        for (auto&& simd_value : simd_values)
        {
#if defined(__AVX512F__)
            auto loaded_simd = left == 0 ?
                                 _mm512_load_si512(
                                   view_as<PatternByte::simd_value_t*>(
                                     &bytes[start_index])) :
                                 _mm512_loadu_si512(
                                   view_as<PatternByte::simd_value_t*>(
                                     &bytes[start_index]));
#elif defined(__AVX2__)
            auto loaded_simd = left == 0 ?
                                 _mm256_load_si256(
                                   view_as<PatternByte::simd_value_t*>(
                                     &bytes[start_index])) :
                                 _mm256_loadu_si256(
                                   view_as<PatternByte::simd_value_t*>(
                                     &bytes[start_index]));
#endif

#if defined(__AVX512F__)
            if (!_mm512_cmpeq_epi64_mask(loaded_simd, simd_value))
            {
                goto skip;
            }
#elif defined(__AVX2__)
            if (!_mm256_movemask_epi8(
                  _mm256_cmpeq_epi64(loaded_simd, simd_value)))
            {
                goto skip;
            }
#else
            if (*view_as<PatternByte::simd_value_t*>(&bytes[+start_index])
                != simd_value)
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
