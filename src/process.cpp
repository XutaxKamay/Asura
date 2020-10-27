#include "process.h"

using namespace XLib;

Process::Process(const std::string& fullName, pid_t pid)
 : _full_name(fullName), _pid(pid)
{
}

auto Process::setFullName(const std::string& fullName) -> void
{
    _full_name = fullName;
}

auto Process::fullName() -> std::string
{
    return _full_name;
}

auto Process::setPID(pid_t pid)
{
    _pid = pid;
}

auto Process::pid() -> pid_t
{
    return _pid;
}
