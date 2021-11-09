#ifndef PROCESSMEMORYAREA_H
#define PROCESSMEMORYAREA_H

#include "memoryarea.h"
#include "memoryutils.h"
#include "processbase.h"

namespace XKLib
{
    class ProcessMemoryArea : public MemoryArea
    {
        private:
            class ModifiableProtectionFlags : private ProtectionFlags
            {
                    friend class ProcessMemoryArea;

                public:
                    explicit ModifiableProtectionFlags(
                      ProcessMemoryArea* _pma);

                    auto change(mapf_t flags) -> mapf_t;

                    auto operator|(mapf_t flags) -> mapf_t;
                    auto operator&(mapf_t flags) -> mapf_t;

                    auto operator=(mapf_t flags)
                      -> ModifiableProtectionFlags&;
                    auto operator|=(mapf_t flags) -> void;
                    auto operator&=(mapf_t flags) -> void;

                public:
                    auto cachedValue() -> mapf_t&;

                private:
                    mapf_t _flags {};

                    ProcessMemoryArea* _pma;
            };

        public:
            explicit ProcessMemoryArea(ProcessBase process);

            auto protectionFlags() -> ModifiableProtectionFlags&;
            auto initProtectionFlags(mapf_t flags) -> void;
            auto processBase() -> ProcessBase;
            auto read(std::size_t size, std::size_t shift = 0) -> bytes_t;
            auto write(const bytes_t& bytes, std::size_t shift = 0)
              -> void;
            auto isDeniedByOS() -> bool;
            auto isReadable() -> bool;
            auto isWritable() -> bool;

        public:
            template <typename T = byte_t>
            auto read() -> std::vector<T>
            {
                if (ProcessBase::self().id() == _process_base.id())
                {
                    std::vector<T> result(
                      MemoryUtils::align_to_page_size(size(), sizeof(T)));
                    std::copy(result.begin(), result.end(), begin<T*>());
                    return result;
                }

                return MemoryUtils::ReadProcessMemoryAreaAligned<T>(
                  _process_base.id(),
                  begin(),
                  size());
            }

        private:
            ModifiableProtectionFlags _protection_flags;
            ProcessBase _process_base;
    };
};

#endif
