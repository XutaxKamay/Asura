#include "processbase.h"

using namespace XLib;

ProcessBase::ProcessBase(pid_t pid) : _pid(pid)
{
}

auto ProcessBase::setPID(pid_t pid)
{
    _pid = pid;
}

auto ProcessBase::pid() -> pid_t
{
    return _pid;
}
