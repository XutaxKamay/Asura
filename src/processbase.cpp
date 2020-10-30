#include "processbase.h"

using namespace XLib;

ProcessBase::ProcessBase(pid_t pid) : _pid(pid)
{
}

auto ProcessBase::setPID(pid_t pid)
{
    _pid = pid;
}

auto ProcessBase::id() -> pid_t
{
    return _pid;
}
