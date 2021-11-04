#include "patternscanning.h"
#include "patternbyte.h"
#include <cstring>

auto XKLib::PatternScanning::search(XKLib::PatternByte& pattern,
                                    const XKLib::bytes_t& bytes,
                                    ptr_t baseAddress) -> bool
{
    auto&& matches        = pattern.matches();
    auto old_matches_size = matches.size();
    auto buffer_size      = bytes.size();
    auto&& unknown_values = pattern.unknown_values();
    auto&& raw_values     = pattern.raw_values();

    for (size_t index = 0; index < buffer_size
                           && (index + raw_values.size()) <= buffer_size;
         index++)
    {
        for (auto&& unknown_value_index : unknown_values)
        {
            raw_values[unknown_value_index] = bytes[index
                                                    + unknown_value_index];
        }

        if (std::memcmp(&bytes[index],
                        raw_values.data(),
                        raw_values.size())
            == 0)
        {
            matches.push_back(
              view_as<ptr_t>(view_as<uintptr_t>(baseAddress) + index));
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
