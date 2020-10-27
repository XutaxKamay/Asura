#ifndef PROCESSMAP_H
#define PROCESSMAP_H

#include "memorymap.h"
#include "memoryutils.h"
#include "processmemoryarea.h"

namespace XLib
{
    class Process;

    class ProcessMemoryMap : public MemoryMap<ProcessMemoryArea>
    {
      public:
        ProcessMemoryMap() = default;
        ProcessMemoryMap(Process* process);

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

      public:
        auto refresh() -> void;

      private:
        Process* _process;
    };
};

#endif
