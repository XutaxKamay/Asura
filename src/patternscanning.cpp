#include "patternscanning.h"
#include "patternbyte.h"

XLib::PatternScanningException::PatternScanningException(
  const std::string& msg)
 : _msg(msg)
{
}

auto XLib::PatternScanningException::msg() -> const std::string&
{
    return _msg;
}

auto XLib::PatternScanning::search(XLib::PatternByte& pattern,
                                   XLib::bytes_t bytes,
                                   ptr_t baseAddress) -> bool
{
    auto& pattern_values  = pattern.values();
    auto old_matches_size = pattern.matches().size();

    /* Let's just skip the first wildcards */
    size_t wildcard_skip = 0;

    while (pattern_values[wildcard_skip].value < 0)
    {
        wildcard_skip++;
    }

    for (size_t index = wildcard_skip; index < bytes.size(); index++)
    {
        /* Check if first byte is the same as we go on first */
        if (bytes[index] == pattern_values[wildcard_skip].value)
        {
            /* Then scan the rest */
            auto start_index = index;

            for (size_t pattern_index = wildcard_skip;
                 pattern_index < pattern_values.size();
                 pattern_index++)
            {
                auto pattern_byte = pattern_values[pattern_index];

                if (start_index >= bytes.size())
                {
                    throw PatternScanningException(
                      std::string(CURRENT_CONTEXT)
                      + "Out of bounds pattern.");
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
        }

    skip:;
    }

    return pattern.matches().size() != old_matches_size;
}

auto XLib::PatternScanning::searchInProcess(XLib::PatternByte& pattern,
                                            Process process) -> void
{
    try
    {
        auto mmap = process.mmap();

        for (auto&& area : mmap.areas())
        {
            if (area->isReadable())
            {
                search(pattern, area->read(), area->begin<ptr_t>());
            }
        }
    }
    catch (MemoryException& me)
    {
        throw me;
    }
    catch (PatternScanningException& pse)
    {
        throw pse;
    }
}

auto XLib::PatternScanning::searchInProcessWithAreaName(
  XLib::PatternByte& pattern,
  Process process,
  const std::string& areaName) -> void
{
    try
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
    catch (MemoryException& me)
    {
        throw me;
    }
    catch (PatternScanningException& pse)
    {
        throw pse;
    }
}
