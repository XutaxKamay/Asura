#include "processmemoryarea.h"
#include "processbase.h"

using namespace XLib;

ProcessMemoryArea::ModifiableProtectionFlags::ModifiableProtectionFlags(
  ProcessMemoryArea* pma)
 : _pma(pma)
{
}

auto ProcessMemoryArea::ModifiableProtectionFlags::change(mapf_t flags)
  -> mapf_t
{
    mapf_t old_flags = _flags;

    try
    {
        MemoryUtils::ProtectMemoryArea(_pma->_process_base.id(),
                                       _pma->begin(),
                                       _pma->size(),
                                       flags);
    }
    catch (MemoryException& me)
    {
        throw me;
    }

    _flags = flags;

    return old_flags;
}

auto ProcessMemoryArea::ModifiableProtectionFlags::cachedValue()
  -> mapf_t&
{
    return _flags;
}

auto ProcessMemoryArea::ModifiableProtectionFlags::operator|(mapf_t flags)
  -> mapf_t
{
    return flags | _flags;
}

auto ProcessMemoryArea::ModifiableProtectionFlags::operator&(mapf_t flags)
  -> mapf_t
{
    return flags & _flags;
}

auto ProcessMemoryArea::ModifiableProtectionFlags::operator=(mapf_t flags)
  -> void
{
    change(flags);
}

auto ProcessMemoryArea::ModifiableProtectionFlags::operator|=(mapf_t flags)
  -> void
{
    change(flags | _flags);
}

auto ProcessMemoryArea::ModifiableProtectionFlags::operator&=(mapf_t flags)
  -> void
{
    change(flags & _flags);
}

ProcessMemoryArea::ProcessMemoryArea(ProcessBase process)
 : _protection_flags(ModifiableProtectionFlags(this)),
   _process_base(process)
{
}

auto ProcessMemoryArea::protectionFlags() -> ModifiableProtectionFlags&
{
    return _protection_flags;
}

auto ProcessMemoryArea::initProtectionFlags(mapf_t flags) -> void
{
    _protection_flags.cachedValue() = flags;
}

auto ProcessMemoryArea::processBase() -> ProcessBase
{
    return _process_base;
}

auto ProcessMemoryArea::read() -> bytes_t
{
    return MemoryUtils::ReadProcessMemoryArea(_process_base.id(),
                                              begin(),
                                              size());
}

auto ProcessMemoryArea::read(size_t size, size_t shift) -> bytes_t
{
    return MemoryUtils::ReadProcessMemoryArea(_process_base.id(),
                                              begin<size_t>() + shift,
                                              size);
}

auto ProcessMemoryArea::write(const bytes_t& bytes, size_t shift) -> void
{
    MemoryUtils::WriteProcessMemoryArea(_process_base.id(),
                                        bytes,
                                        begin<size_t>() + shift);
}
