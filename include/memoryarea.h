#ifndef MEMORYAREA_H
#define MEMORYAREA_H

#include "types.h"

namespace XLib
{
    /**
     * @brief memory_protection_flags_t
     * Modern OS have generally protections on each memory areas,
     * combined of these three flags:
     * READ, WRITE, EXECUTE
     */
    enum memory_protection_flags_t
    {
        NONE    = 0,
        READ    = (1 << 0),
        WRITE   = (1 << 1),
        EXECUTE = (1 << 2),
    };

    /**
     * @brief MemoryArea
     * Class that permits to handle the memory area.
     */
    class MemoryArea
    {
      public:
        class Protection
        {
          public:
            static auto toOwn(int flags) -> memory_protection_flags_t;
            static auto toOS(memory_protection_flags_t flags) -> int;
        };

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

        /**
         * The current protection.
         */
        auto protection() -> memory_protection_flags_t&;

        /**
         * The default flags that were initialized with.
         */
        auto setDefaultProtectionFlags(memory_protection_flags_t flags)
          -> void;

      public:
        template <typename T = uintptr_t>
        auto begin() const
        {
            return view_as<T>(_address);
        }

        template <typename T = uintptr_t>
        auto end() const
        {
            return view_as<T>(view_as<uintptr_t>(_address) + _size);
        }

        template <typename T = size_t>
        auto size() const
        {
            return view_as<T>(end<size_t>() - begin<size_t>());
        }

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
         * @brief _flags
         * Protection flags on the memory area.
         */
        memory_protection_flags_t _flags {};
        memory_protection_flags_t _default_flags {};
    };
}

#endif
