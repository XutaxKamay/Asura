#include "pch.h"

#include "exception.h"

#include "kokabiel.h"

XKLib::Kokabiel::Kokabiel(const std::string& fileName)
{
    if (!_elf.load(fileName))
    {
        XKLIB_EXCEPTION("Couldn't load " + fileName);
    }
}

auto XKLib::Kokabiel::loadSegments() -> void
{
    if (!_loaded_segments.empty())
    {
        return;
    }

    /* copy segments */
    for (auto&& segment : _elf.segments)
    {
        auto seg_type = segment->get_type();

        if (seg_type == ELFIO::PT_LOAD)
        {
            loadable_segment_t loaded_segment;

            loaded_segment.start = view_as<ptr_t>(
              MemoryUtils::Align(segment->get_virtual_address(),
                                 MemoryUtils::GetPageSize()));

            auto left_over = segment->get_virtual_address()
                             - view_as<uintptr_t>(loaded_segment.start);

            auto memory_aligned_size = MemoryUtils::AlignToPageSize(
              segment->get_memory_size() + left_over,
              MemoryUtils::GetPageSize());

            loaded_segment.data.resize(memory_aligned_size);

            std::copy(segment->get_data(),
                      segment->get_data() + segment->get_file_size(),
                      loaded_segment.data.begin() + left_over);

            auto seg_flags = segment->get_flags();

            loaded_segment.flags = ((seg_flags & ELFIO::PF_R) ?
                                      MemoryArea::ProtectionFlags::R :
                                      0)
                                   | ((seg_flags & ELFIO::PF_W) ?
                                        MemoryArea::ProtectionFlags::W :
                                        0)
                                   | ((seg_flags & ELFIO::PF_X) ?
                                        MemoryArea::ProtectionFlags::X :
                                        0);

            _loaded_segments.push_back(loaded_segment);
        }
    }

    if (_loaded_segments.empty())
    {
        XKLIB_EXCEPTION("No loadable segments inside the elf file");
    }

    /* sort segments */
    std::sort(_loaded_segments.begin(),
              _loaded_segments.end(),
              [](const loadable_segment_t& rs1,
                 const loadable_segment_t& rs2)
              {
                  return view_as<uintptr_t>(rs1.start)
                         < view_as<uintptr_t>(rs2.start);
              });
}
