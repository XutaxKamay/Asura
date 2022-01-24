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

ProcessBase::ProcessBase(process_id_t pid) : _pid(pid)
{
}

auto ProcessBase::setPID(process_id_t pid)
{
    _pid = pid;
}

auto ProcessBase::id() -> process_id_t
{
    return _pid;
}
