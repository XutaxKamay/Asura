#include "pch.h"

#include "processbase.h"

using namespace XKLib;

auto ProcessBase::self() -> ProcessBase
{
#ifdef WINDOWS
    return ProcessBase(GetCurrentProcessId());
#else
    return ProcessBase(getpid());
#endif
}

ProcessBase::ProcessBase(const process_id_t pid)
 : _pid(pid)
{
}

auto ProcessBase::setPID(const process_id_t pid) -> void
{
    _pid = pid;
}

auto ProcessBase::id() const -> process_id_t
{
    return _pid;
}
