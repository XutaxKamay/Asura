#include "patternscanning.h"
#include "patternbyte.h"

auto XKLib::PatternScanning::search(XKLib::PatternByte& pattern,
                                    XKLib::bytes_t bytes,
                                    ptr_t baseAddress) -> bool
{
    auto& pattern_values  = pattern.values();
    auto old_matches_size = pattern.matches().size();

    for (size_t index = 0; index < bytes.size(); index++)
    {
        /* Then scan the rest */
        auto start_index = index;

        for (size_t pattern_index = 0;
             pattern_index < pattern_values.size();
             pattern_index++)
        {
            auto pattern_byte = pattern_values[pattern_index];

            if (start_index >= bytes.size())
            {
                XKLIB_EXCEPTION("Out of bounds pattern.");
            }
            else if (pattern_byte.value
                     == PatternByte::Value::type_t::UNKNOWN)
            {
                start_index++;
                continue;
            }
            else if (pattern_byte.value != bytes[start_index])
            {
                goto skip;
            }

            start_index++;
        }

        pattern.matches().push_back(
          view_as<ptr_t>(view_as<uintptr_t>(baseAddress) + index));

    skip:;
    }

    return pattern.matches().size() != old_matches_size;
}

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
