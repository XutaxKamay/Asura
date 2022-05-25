#include "pch.h"

#include "exception.h"
#include "memoryutils.h"

#include "kokabiel.h"

XKLib::Kokabiel::Kokabiel(const std::string& fileName)
{
    if (not _elf.load(fileName))
    {
        XKLIB_EXCEPTION("Couldn't load " + fileName);
    }

    loadSegments();
}

auto XKLib::Kokabiel::loadSegments() -> void
{
    /* copy segments */
    for (const auto& segment : _elf.segments)
    {
        const auto seg_type = segment->get_type();

        if (seg_type == ELFIO::PT_LOAD)
        {
            MemoryArea loadable_segment;

            loadable_segment.start = MemoryUtils::Align(
              segment->get_virtual_address(),
              MemoryUtils::GetPageSize());

            const auto left_over = segment->get_virtual_address()
                                   - loadable_segment.start;

            const auto memory_aligned_size = MemoryUtils::AlignToPageSize(
              segment->get_memory_size() + left_over,
              MemoryUtils::GetPageSize());

            loadable_segment.bytes.resize(memory_aligned_size);

            std::copy(segment->get_data(),
                      segment->get_data() + segment->get_file_size(),
                      view_as<byte_t*>(loadable_segment.bytes.data()
                                       + left_over));

            const auto seg_flags = segment->get_flags();

            loadable_segment.flags = ((seg_flags & ELFIO::PF_R) ? XKLib::
                                          MemoryArea::ProtectionFlags::R :
                                                                  0)
                                     | ((seg_flags & ELFIO::PF_W) ?
                                          XKLib::MemoryArea::
                                            ProtectionFlags::W :
                                          0)
                                     | ((seg_flags & ELFIO::PF_X) ?
                                          XKLib::MemoryArea::
                                            ProtectionFlags::X :
                                          0);

            _loadable_segments.push_back(loadable_segment);
        }
    }

    if (_loadable_segments.empty())
    {
        XKLIB_EXCEPTION("No loadable segments inside the elf file");
    }

    /* sort segments */
    std::sort(_loadable_segments.begin(),
              _loadable_segments.end(),
              [](const MemoryArea& ma1, const MemoryArea& ma2)
              {
                  return ma1.start < ma2.start;
              });

    const auto last = _loadable_segments.end() - 1;

    _image_size = (last->start + last->bytes.size())
                  - _loadable_segments.begin()->start;
}

auto XKLib::Kokabiel::freeInjection(InjectionInfo& injectionInfo) const
  -> void
{
    injectionInfo.process_memory_map.freeArea(
      injectionInfo.allocated_mem.shellcode.start,
      injectionInfo.allocated_mem.shellcode.bytes.size());

    injectionInfo.process_memory_map.freeArea(
      injectionInfo.allocated_mem.env_data.start,
      injectionInfo.allocated_mem.env_data.bytes.size()
        + MemoryUtils::GetPageSize());

    injectionInfo.process_memory_map.freeArea(
      injectionInfo.loaded_segments.begin()->start,
      _image_size);
}
