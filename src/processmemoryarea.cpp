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
        MemoryUtils::ProtectMemoryArea(_pma->_process->pid(),
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

auto ProcessMemoryArea::ModifiableProtectionFlags::defaultValue()
  -> mapf_t&
{
    return _default_flags;
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

ProcessMemoryArea::ProcessMemoryArea(ProcessBase* process)
 : _protection_flags(ModifiableProtectionFlags(this)), _process(process)
{
}

auto ProcessMemoryArea::protectionFlags() -> ModifiableProtectionFlags&
{
    return _protection_flags;
}

auto ProcessMemoryArea::resetToDefaultFlags() -> mapf_t
{
    return _protection_flags.change(_protection_flags.cachedValue());
}

auto ProcessMemoryArea::initProtectionFlags(mapf_t flags) -> void
{
    _protection_flags.cachedValue()  = flags;
    _protection_flags.defaultValue() = flags;
}

auto ProcessMemoryArea::process() -> ProcessBase*
{
    return _process;
}

auto ProcessMemoryArea::read() -> bytes_t
{
    return MemoryUtils::ReadProcessMemoryArea(_process->pid(),
                                              begin(),
                                              size());
}

auto ProcessMemoryArea::write(const bytes_t& bytes) -> void
{
    MemoryUtils::WriteProcessMemoryArea(_process->pid(), bytes, begin());
}
