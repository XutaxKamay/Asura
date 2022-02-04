#include "pch.h"

#include "exception.h"

#include "kokabiel.h"

XKLib::Kokabiel::Kokabiel(const std::string& fileName)
{
    if (!_elf.load(fileName))
    {
        XKLIB_EXCEPTION("Couldn't load " + fileName);
    }

    loadSegments();
}

auto XKLib::Kokabiel::loadSegments() -> void
{
    /* copy segments */
    for (auto&& segment : _elf.segments)
    {
        auto seg_type = segment->get_type();

        if (seg_type == ELFIO::PT_LOAD)
        {
            memory_area_t loadable_segment;

            loadable_segment.start = MemoryUtils::Align(
              segment->get_virtual_address(),
              MemoryUtils::GetPageSize());

            auto left_over = segment->get_virtual_address()
                             - loadable_segment.start;

            auto memory_aligned_size = MemoryUtils::AlignToPageSize(
              segment->get_memory_size() + left_over,
              MemoryUtils::GetPageSize());

            loadable_segment.bytes.resize(memory_aligned_size);

            std::copy(segment->get_data(),
                      segment->get_data() + segment->get_file_size(),
                      view_as<byte_t*>(loadable_segment.bytes.data()
                                       + left_over));

            auto seg_flags = segment->get_flags();

            loadable_segment.flags = ((seg_flags & ELFIO::PF_R) ?
                                        MemoryArea::ProtectionFlags::R :
                                        0)
                                     | ((seg_flags & ELFIO::PF_W) ?
                                          MemoryArea::ProtectionFlags::W :
                                          0)
                                     | ((seg_flags & ELFIO::PF_X) ?
                                          MemoryArea::ProtectionFlags::X :
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
              [](const memory_area_t& ma1, const memory_area_t& ma2)
              {
                  return ma1.start < ma2.start;
              });

    auto last = _loadable_segments.end() - 1;

    _image_size = (last->start + last->bytes.size())
                  - _loadable_segments.begin()->start;
}

auto XKLib::Kokabiel::freeInjection(injection_info_t& injectionInfo)
  -> void
{
    injectionInfo.processMemoryMap.freeArea(
      injectionInfo.allocated_mem.shellcode.start,
      injectionInfo.allocated_mem.shellcode.bytes.size());

    injectionInfo.processMemoryMap.freeArea(
      injectionInfo.allocated_mem.env_data.start,
      injectionInfo.allocated_mem.env_data.bytes.size());

    injectionInfo.processMemoryMap.freeArea(
      injectionInfo.loaded_segments.begin()->start,
      _image_size);
}
