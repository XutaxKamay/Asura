#include "process.h"

using namespace XLib;

Process::Process(const maps_t& maps, const std::string fullName, pid_t pid)
 : _maps(maps), _full_name(fullName), _pid(pid)
{
}

auto Process::refresh() -> void
{
    _maps = MemoryUtils::QueryMaps(_pid);
}

auto Process::maps()
{
    refresh();
    return _maps;
}

auto Process::setFullName(const std::string& fullName) -> void
{
    _full_name = fullName;
}

auto Process::fullName() const
{
    return _full_name;
}

auto Process::setPID(pid_t pid)
{
    _pid = pid;
    refresh();
}

auto Process::pid()
{
    return _pid;
}
