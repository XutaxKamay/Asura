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

      public:
        auto setFullName(const std::string& fullName) -> void;
        auto fullName() -> std::string;

        auto setPID(pid_t pid);
        auto pid() -> pid_t;

        auto mmap() -> ProcessMemoryMap&;

      public:
        template <typename T = uintptr_t>
        auto allocArea(T address,
                       size_t size,
                       memory_protection_flags_t flags) -> void;

        template <typename T = uintptr_t>
        auto freeArea(T address, size_t size) -> void;

        template <typename T = uintptr_t>
        auto protectMemoryArea(T address,
                               size_t size,
                               memory_protection_flags_t flags) -> void;

        template <typename T>
        auto read(T address, size_t size) -> bytes_t;

        template <typename T>
        auto write(T address, const bytes_t& bytes) -> void;

      private:
        std::string _full_name {};
        pid_t _pid {};
        ProcessMemoryMap _mmap {};
    };
} // namespace XLib

#endif
