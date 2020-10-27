#ifndef PROCESSMAP_H
#define PROCESSMAP_H

#include "memorymap.h"
#include "memoryutils.h"
#include "process.h"
#include "processmemoryarea.h"

namespace XLib
{
    class ProcessMemoryMap : public MemoryMap<ProcessMemoryArea>
    {
      public:
        ProcessMemoryMap() = default;
        ProcessMemoryMap(Process* process);

        template <typename T = uintptr_t>
        auto allocArea(T address,
                       size_t size,
                       memory_protection_flags_t flags)
        {
            return MemoryUtils::AllocArea<T>(_process->pid(),
                                             address,
                                             size,
                                             flags);
        }

        template <typename T = uintptr_t>
        auto freeArea(T address, size_t size)
        {
            return MemoryUtils::FreeArea(_process->pid(), address, size);
        }

        template <typename T = uintptr_t>
        auto protectMemoryArea(T address,
                               size_t size,
                               memory_protection_flags_t flags)
        {
            return MemoryUtils::ProtectMemoryArea(_process->pid(),
                                                  address,
                                                  size,
                                                  flags);
        }

      public:
        auto fetch() -> void;

      private:
        Process* _process;
    };
};

#endif
