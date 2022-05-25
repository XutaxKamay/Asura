#include "pch.h"

#include "processbase.h"
#include "processmemoryarea.h"

using namespace XKLib;

ProcessMemoryArea::ModifiableProtectionFlags::ModifiableProtectionFlags(
  ProcessMemoryArea* const pma)
 : _pma(pma)
{
}

auto ProcessMemoryArea::ModifiableProtectionFlags::change(
  const mapf_t flags) -> mapf_t
{
    mapf_t old_flags = _flags;

    try
    {
        MemoryUtils::ProtectMemoryArea(_pma->_process_base.id(),
                                       _pma->begin(),
                                       _pma->size(),
                                       flags);

        _flags = flags;
    }
    catch (Exception& e)
    {
        throw Exception(e.msg());
    }

    return old_flags;
}

auto ProcessMemoryArea::ModifiableProtectionFlags::cachedValue() const
  -> const mapf_t&
{
    return _flags;
}

auto ProcessMemoryArea::ModifiableProtectionFlags::cachedValue()
  -> mapf_t&
{
    return _flags;
}

auto ProcessMemoryArea::ModifiableProtectionFlags::operator=(mapf_t flags)
  -> void
{
    change(flags);
}

ProcessMemoryArea::ProcessMemoryArea(ProcessBase processBase)
 : _protection_flags(ModifiableProtectionFlags(this)),
   _process_base(std::move(processBase))
{
}

auto ProcessMemoryArea::protectionFlags() const
  -> const ModifiableProtectionFlags&
{
    return _protection_flags;
}

auto ProcessMemoryArea::processBase() const -> const ProcessBase&
{
    return _process_base;
}

auto ProcessMemoryArea::read(std::size_t size, std::size_t shift) const
  -> bytes_t
{
    if (ProcessBase::self().id() == _process_base.id())
    {
        return bytes_t(&begin<data_t>()[shift],
                       &begin<data_t>()[shift + size]);
    }

    return MemoryUtils::ReadProcessMemoryArea(_process_base.id(),
                                              begin<std::size_t>()
                                                + shift,
                                              size);
}

auto ProcessMemoryArea::write(const bytes_t& bytes,
                              std::size_t shift) const -> void
{
    if (ProcessBase::self().id() == _process_base.id())
    {
        std::copy(bytes.begin(), bytes.end(), begin<data_t>() + shift);
        return;
    }

    MemoryUtils::WriteProcessMemoryArea(_process_base.id(),
                                        bytes,
                                        begin<std::size_t>() + shift);
}

auto XKLib::ProcessMemoryArea::isDeniedByOS() const -> bool
{
#ifndef WIN32
    return name() == "[vvar]";
#else
    return false;
#endif
}

auto XKLib::ProcessMemoryArea::isReadable() const -> bool
{
    return (_protection_flags.cachedValue() & ProtectionFlags::R)
           and not isDeniedByOS();
}

auto XKLib::ProcessMemoryArea::isWritable() const -> bool
{
    return (_protection_flags.cachedValue() & ProtectionFlags::W)
           and not isDeniedByOS();
}

auto ProcessMemoryArea::protectionFlags() -> ModifiableProtectionFlags&
{
    return _protection_flags;
}

auto ProcessMemoryArea::initProtectionFlags(mapf_t flags) -> void
{
    _protection_flags.cachedValue() = flags;
}
