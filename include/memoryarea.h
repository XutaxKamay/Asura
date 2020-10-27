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

      public:
        template <typename T = uintptr_t>
        auto begin() -> T;

        template <typename T = uintptr_t>
        auto end() -> T;

        template <typename T = size_t>
        auto size() -> T;

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
    };
}

#endif
