#ifndef ASURA_MEMORYAREA_H
#define ASURA_MEMORYAREA_H

#include "types.h"

namespace Asura
{
    /* memory area protection flags */
#ifdef WINDOWS
    using mapf_t = DWORD;
#else
    using mapf_t = std::uint32_t;
#endif

    class MemoryArea
    {
      public:
        class ProtectionFlags
        {
          public:
            static auto ToOwn(const mapf_t flags) -> mapf_t;
            static auto ToOS(const mapf_t flags) -> mapf_t;

            const inline static mapf_t NONE    = 0u;
            const inline static mapf_t READ    = (1u << 0u);
            const inline static mapf_t WRITE   = (1u << 1u);
            const inline static mapf_t EXECUTE = (1u << 2u);
            const inline static mapf_t RWX     = READ | WRITE | EXECUTE;
            const inline static mapf_t RX      = READ | EXECUTE;
            const inline static mapf_t WX      = WRITE | EXECUTE;
            const inline static mapf_t RW      = READ | WRITE;
            const inline static mapf_t R       = READ;
            const inline static mapf_t W       = WRITE;
            const inline static mapf_t X       = EXECUTE;
        };

      public:
        auto operator==(const MemoryArea& area) const -> bool;
        auto name() const -> const std::string&;

      public:
        auto setAddress(const ptr_t address) -> void;
        auto setSize(const std::size_t size) -> void;
        auto setName(const std::string& name) -> void;

      public:
        template <typename T = std::uintptr_t>
        auto begin() const -> T
        {
            return view_as<T>(_address);
        }

        template <typename T = std::uintptr_t>
        auto end() const -> T
        {
            return view_as<T>(view_as<std::uintptr_t>(_address) + _size);
        }

        template <typename T = std::uintptr_t>
        auto size() const -> T
        {
            return view_as<T>(end<std::uintptr_t>()
                              - begin<std::uintptr_t>());
        }

      private:
        ptr_t _address {};
        std::size_t _size {};
        std::string _name {};
    };
}

#endif
