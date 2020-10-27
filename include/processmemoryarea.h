#ifndef PROCESSMEMORYAREA_H
#define PROCESSMEMORYAREA_H

#include "memoryarea.h"
#include "memoryutils.h"

namespace XLib
{
    class Process;
    class ProcessMemoryArea : public MemoryArea
    {
      private:
        class ModifiableProtection : private Protection
        {
          public:
            ModifiableProtection(ProcessMemoryArea* _pma);

            auto change(memory_protection_flags_t flags)
              -> memory_protection_flags_t;

            auto flags() -> memory_protection_flags_t&;
            auto defaultFlags() -> memory_protection_flags_t&;

            auto operator|(memory_protection_flags_t flags)
              -> memory_protection_flags_t;
            auto operator&(memory_protection_flags_t flags)
              -> memory_protection_flags_t;

            auto operator=(memory_protection_flags_t flags) -> void;
            auto operator|=(memory_protection_flags_t flags) -> void;
            auto operator&=(memory_protection_flags_t flags) -> void;

          private:
            memory_protection_flags_t _flags {};
            memory_protection_flags_t _default_flags {};

          private:
            ProcessMemoryArea* _pma;
        };

      public:
        ProcessMemoryArea(Process* process);

        auto protection() -> ModifiableProtection&;

        auto resetToDefaultFlags() -> memory_protection_flags_t;

        auto initProtectionFlags(memory_protection_flags_t flags) -> void;

        auto process() -> Process*;

        auto read() -> bytes_t;
        auto write(const bytes_t& bytes) -> void;

      private:
        ModifiableProtection _protection;
        Process* _process;
    };
};

#endif
