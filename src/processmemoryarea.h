#ifndef PROCESSMEMORYAREA_H
#define PROCESSMEMORYAREA_H

#include "memoryarea.h"
#include "memoryutils.h"
#include "processbase.h"

namespace XLib
{
    class ProcessMemoryArea : public MemoryArea
    {
      private:
        class ModifiableProtectionFlags : private ProtectionFlags
        {
            friend class ProcessMemoryArea;

          public:
            ModifiableProtectionFlags(ProcessMemoryArea* _pma);

            auto change(mapf_t flags) -> mapf_t;

            auto operator|(mapf_t flags) -> mapf_t;
            auto operator&(mapf_t flags) -> mapf_t;

            auto operator=(mapf_t flags) -> void;
            auto operator|=(mapf_t flags) -> void;
            auto operator&=(mapf_t flags) -> void;

          public:
            auto cachedValue() -> mapf_t&;

          private:
            mapf_t _flags {};

            ProcessMemoryArea* _pma;
        };

      public:
        ProcessMemoryArea(ProcessBase process);

        auto protectionFlags() -> ModifiableProtectionFlags&;
        auto initProtectionFlags(mapf_t flags) -> void;
        auto processBase() -> ProcessBase;
        auto read() -> bytes_t;
        auto write(const bytes_t& bytes) -> void;

      private:
        ModifiableProtectionFlags _protection_flags;
        ProcessBase _process_base;
    };
};

#endif
