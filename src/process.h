#ifndef PROCESS_H
#define PROCESS_H

#include "memorymap.h"
#include "memoryutils.h"
#include "processbase.h"
#include "processmemoryarea.h"
#include "processmemorymap.h"

namespace XLib
{
    class Process : public ProcessBase
    {
      public:
        Process();
        Process(const std::string& fullName, pid_t pid);

      public:
        auto setFullName(const std::string& fullName) -> void;
        auto fullName() -> std::string;

        auto mmap() -> ProcessMemoryMap&;

      public:
        template <typename T = uintptr_t>
        auto allocArea(T address, size_t size, mapf_t flags) -> ptr_t
        {
            return _mmap.allocArea<T>(address, size, flags);
        }

        template <typename T = uintptr_t>
        auto freeArea(T address, size_t size) -> void
        {
            _mmap.freeArea<T>(address, size);
        }

        template <typename T = uintptr_t>
        auto protectMemoryArea(T address, size_t size, mapf_t flags)
          -> void
        {
            _mmap.protectMemoryArea(address, size, flags);
        }

        template <typename T = uintptr_t>
        auto read(T address, size_t size) -> bytes_t
        {
            return _mmap.read(address, size);
        }

        template <typename T = uintptr_t>
        auto write(T address, const bytes_t& bytes) -> void
        {
            _mmap.write(address, bytes);
        }

        template <typename T = uintptr_t>
        auto forceWrite(T address, const bytes_t& bytes) -> void
        {
            _mmap.forceWrite(address, bytes);
        }

      private:
        std::string _full_name {};
        ProcessMemoryMap _mmap {};
    };
}

#endif
