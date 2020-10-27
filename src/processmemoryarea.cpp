#include "processmemoryarea.h"
#include "process.h"

using namespace XLib;

ProcessMemoryArea::ModifiableProtection::ModifiableProtection(
  ProcessMemoryArea* pma)
 : _pma(pma)
{
}

auto ProcessMemoryArea::ModifiableProtection::change(
  memory_protection_flags_t flags) -> memory_protection_flags_t
{
    memory_protection_flags_t old_flags = _flags;

    try
    {
        _pma->process()->protectMemoryArea(_pma->begin(),
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

auto ProcessMemoryArea::ModifiableProtection::defaultFlags()
  -> memory_protection_flags_t&
{
    return _default_flags;
}

auto ProcessMemoryArea::ModifiableProtection::flags()
  -> memory_protection_flags_t&
{
    return _flags;
}

auto ProcessMemoryArea::ModifiableProtection::operator|(
  memory_protection_flags_t flags) -> memory_protection_flags_t
{
    return view_as<memory_protection_flags_t>(flags | _flags);
}

auto ProcessMemoryArea::ModifiableProtection::operator&(
  memory_protection_flags_t flags) -> memory_protection_flags_t
{
    return view_as<memory_protection_flags_t>(flags & _flags);
}

auto ProcessMemoryArea::ModifiableProtection::operator=(
  memory_protection_flags_t flags) -> void
{
    change(flags);
}

auto ProcessMemoryArea::ModifiableProtection::operator|=(
  memory_protection_flags_t flags) -> void
{
    change(view_as<memory_protection_flags_t>(flags | _flags));
}

auto ProcessMemoryArea::ModifiableProtection::operator&=(
  memory_protection_flags_t flags) -> void
{
    change(view_as<memory_protection_flags_t>(flags & _flags));
}

ProcessMemoryArea::ProcessMemoryArea(Process* process)
 : _protection(ModifiableProtection(this)), _process(process)
{
}

auto ProcessMemoryArea::protection() -> ModifiableProtection&
{
    return _protection;
}

auto ProcessMemoryArea::resetToDefaultFlags() -> memory_protection_flags_t
{
    return _protection.change(_protection.defaultFlags());
}

auto ProcessMemoryArea::initProtectionFlags(
  memory_protection_flags_t flags) -> void
{
    _protection.defaultFlags() = flags;
    _protection.flags()        = flags;
}

auto ProcessMemoryArea::process() -> Process*
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
    return MemoryUtils::WriteProcessMemoryArea(_process->pid(),
                                               bytes,
                                               begin());
}
