#ifndef PROCESS_H
#define PROCESS_H

#include "processmemorymap.h"

namespace XLib
{
    class Process
    {
      public:
        Process() = default;
        Process(const std::string& fullName, pid_t pid);

        auto setFullName(const std::string& fullName) -> void;
        auto fullName() -> std::string;

        auto setPID(pid_t pid);
        auto pid() -> pid_t;

        template <typename T = uintptr_t>
        auto allocArea(T address,
                       size_t size,
                       memory_protection_flags_t flags)
        {
            return _mmap.allocArea<T>(address, size, flags);
        }

        template <typename T = uintptr_t>
        auto freeArea(T address, size_t size)
        {
            return _mmap.freeArea<T>(address, size);
        }

        template <typename T = uintptr_t>
        auto protectMemoryArea(T address,
                               size_t size,
                               memory_protection_flags_t flags)
        {
            return _mmap.protectMemoryArea(address, size, flags);
        }

      private:
        std::string _full_name {};
        pid_t _pid {};
        ProcessMemoryMap _mmap {};
    };
} // namespace XLib

#endif
