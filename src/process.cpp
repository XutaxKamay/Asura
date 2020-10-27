#include "process.h"

using namespace XLib;

Process::Process(const std::string& fullName, pid_t pid)
 : _full_name(fullName), _pid(pid), _mmap(ProcessMemoryMap(this))
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

auto Process::mmap() -> ProcessMemoryMap&
{
    _mmap.refresh();
    return _mmap;
}

template <typename T>
auto Process::allocArea(T address,
                        size_t size,
                        memory_protection_flags_t flags) -> void
{
    _mmap.allocArea<T>(address, size, flags);
}

template <typename T>
auto Process::freeArea(T address, size_t size) -> void
{
    _mmap.freeArea<T>(address, size);
}

template <typename T>
auto Process::protectMemoryArea(T address,
                                size_t size,
                                memory_protection_flags_t flags) -> void
{
    _mmap.protectMemoryArea(address, size, flags);
}

template auto Process::protectMemoryArea<uintptr_t>(
  uintptr_t,
  size_t,
  memory_protection_flags_t) -> void;

template auto Process::protectMemoryArea<ptr_t>(ptr_t,
                                                size_t,
                                                memory_protection_flags_t)
  -> void;
