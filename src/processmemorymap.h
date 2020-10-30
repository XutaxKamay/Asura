#ifndef PROCESSMEMORYMAP_H
#define PROCESSMEMORYMAP_H

#include "memorymap.h"
#include "memoryutils.h"
#include "processbase.h"
#include "processmemoryarea.h"

namespace XLib
{
    class ProcessMemoryArea;
    class ProcessMemoryMap : public MemoryMap<ProcessMemoryArea>
    {
      public:
        ProcessMemoryMap() = default;
        ProcessMemoryMap(ProcessBase* process);

      public:
        auto refresh() -> void;

      public:
        template <typename T = uintptr_t>
        auto allocArea(T address, size_t size, mapf_t flags) -> ptr_t
        {
            auto ret = MemoryUtils::AllocArea(_process_base->id(),
                                              address,
                                              size,
                                              flags);

            refresh();

            return ret;
        }

        template <typename T = uintptr_t>
        auto freeArea(T address, size_t size) -> void
        {
            MemoryUtils::FreeArea(_process_base->id(), address, size);

            refresh();
        }

        template <typename T = uintptr_t>
        auto protectMemoryArea(T address, size_t size, mapf_t flags)
          -> void
        {
            MemoryUtils::ProtectMemoryArea(_process_base->id(),
                                           address,
                                           size,
                                           flags);

            refresh();
        }

        template <typename T = uintptr_t>
        auto read(T address, size_t size) -> bytes_t
        {
            return MemoryUtils::ReadProcessMemoryArea(_process_base->id(),
                                                      address,
                                                      size);
        }

        template <typename T = uintptr_t>
        auto write(T address, const bytes_t& bytes) -> void
        {
            MemoryUtils::WriteProcessMemoryArea(_process_base->id(),
                                                address,
                                                bytes);
        }

        template <typename T = uintptr_t>
        auto forceWrite(T address, const bytes_t& bytes) -> void
        {
            refresh();

            auto area = search(address);

            if (!area)
            {
                throw MemoryException(std::string(CURRENT_CONTEXT)
                                      + "Could not find area");
            }

            area->protectionFlags() |= MemoryArea::ProtectionFlags::W;

            MemoryUtils::WriteProcessMemoryArea(_process_base->id(),
                                                address,
                                                bytes);

            area->resetToDefaultFlags();
        }

      private:
        ProcessBase* _process_base;
    };
};

#endif
