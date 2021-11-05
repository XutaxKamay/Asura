#ifndef MEMORYAREA_H
#define MEMORYAREA_H

#include "types.h"

namespace XKLib
{
    /**
     * memory area protection flags
     */
#ifdef WINDOWS
    using mapf_t = DWORD;
#else
    using mapf_t = uint32_t;
#endif

    /**
     * @brief MemoryArea
     * Class that permits to handle the memory area.
     */
    class MemoryArea
    {
      public:
        class ProtectionFlags
        {
          public:
            static auto ToOwn(mapf_t flags) -> mapf_t;
            static auto ToOS(mapf_t flags) -> mapf_t;

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
        /**
         * @brief setAddress
         *
         * @param address
         */
        auto setAddress(ptr_t address) -> void;
        /**
         * @brief setSize
         *
         * @param size
         */
        auto setSize(size_t size) -> void;
        auto setName(const std::string& name) -> void;
        auto operator==(MemoryArea& area) -> bool;

      public:
        template <typename T = uintptr_t>
        auto begin() -> T
        {
            return view_as<T>(_address);
        }

        template <typename T = uintptr_t>
        auto end() -> T
        {
            return view_as<T>(view_as<uintptr_t>(_address) + _size);
        }

        template <typename T = uintptr_t>
        auto size() -> T
        {
            return view_as<T>(end<size_t>() - begin<size_t>());
        }

        auto name() -> const std::string&;

      private:
        /**
         * @brief _address
         * Address of the memory area
         */
        ptr_t _address {};
        /**
         * @brief _size
         * Size of the memory area
         */
        size_t _size {};
        /**
         * @brief _name
         * Name of the memory area
         */
        std::string _name {};
    };
}

#endif
