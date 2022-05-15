#ifndef XKLIB_PROCESSMEMORYAREA_H
#define XKLIB_PROCESSMEMORYAREA_H

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
              ProcessMemoryArea* const _pma);

          public:
            auto cachedValue() const -> const mapf_t&;

          public:
            auto cachedValue() -> mapf_t&;
            auto change(const mapf_t flags) -> mapf_t;
            auto operator=(const mapf_t flags) -> void;

          private:
            mapf_t _flags {};

            ProcessMemoryArea* _pma;
        };

      public:
        explicit ProcessMemoryArea(ProcessBase processBase);

        auto protectionFlags() const -> const ModifiableProtectionFlags&;
        auto processBase() const -> const ProcessBase&;
        auto read(const std::size_t size,
                  const std::size_t shift = 0) const -> bytes_t;
        auto write(const bytes_t& bytes,
                   const std::size_t shift = 0) const -> void;
        auto isDeniedByOS() const -> bool;
        auto isReadable() const -> bool;
        auto isWritable() const -> bool;

      public:
        auto protectionFlags() -> ModifiableProtectionFlags&;
        auto initProtectionFlags(const mapf_t flags) -> void;

      public:
        template <typename T = byte_t>
        auto readAligned() const -> std::vector<T>
        {
            if (ProcessBase::self().id() == _process_base.id())
            {
                /**
                 * Be sure there's enough memory for pattern scanning to
                 * avoid crashes
                 */
                const std::vector<T> result(
                  MemoryUtils::AlignToPageSize(size() + (sizeof(T) * 2),
                                               sizeof(T)));
                std::copy(result.begin() + 1,
                          result.end() - 1,
                          begin<T*>());
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
