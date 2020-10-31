#include "process.h"

using namespace XLib;

Process::Process() : ProcessBase()
{
}

Process::Process(const std::string& fullName, pid_t pid)
 : ProcessBase(pid), _full_name(fullName), _mmap(ProcessMemoryMap(this))
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

auto Process::tasks() -> tasks_t
{
    return Task::list(this);
}

auto Process::mmap() -> ProcessMemoryMap&
{
    _mmap.refresh();
    return _mmap;
}
