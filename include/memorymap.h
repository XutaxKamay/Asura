#ifndef MEMORYMAP_H
#define MEMORYMAP_H

#include "types.h"

namespace XLib
{
    template <typename T>
    /**
     * @brief MemoryArea
     * Class that permits to handle the memory memory_map.
     */
    class MemoryArea
    {
      public:
        /**
         * @brief protection_t
         * Modern OS have generally protections on each memory memory_map,
         * combined of these three flags:
         * READ, WRITE, EXECUTE
         */
        enum protection_t : byte_t
        {
            NONE    = 0,
            READ    = (1 << 0),
            WRITE   = (1 << 1),
            EXECUTE = (1 << 2),
        };

        /**
         * @brief begin
         *
         * @return auto
         */
        auto begin();
        /**
         * @brief begin
         *
         * @return auto
         */
        auto end();
        /**
         * @brief setAddress
         *
         * @param address
         */
        auto setAddress(const ptr_t& address) -> void;
        /**
         * @brief setSize
         *
         * @param size
         */
        auto setSize(const size_t& size) -> void;
        /**
         * @brief protection
         *
         * @return auto&
         */
        auto& protection();

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
         * @brief _protection
         * Protection flags on the memory area.
         */
        protection_t _protection {};
    };

    using memory_area_t = MemoryArea<ptr_t>;
    using memory_map_t  = std::vector<memory_area_t>;

    template <typename T>
    auto MemoryArea<T>::begin()
    {
        return view_as<T>(_address);
    }

    template <typename T>
    auto MemoryArea<T>::end()
    {
        return view_as<T>(view_as<uintptr_t>(_address) + _size);
    }

    template <typename T>
    auto MemoryArea<T>::setAddress(const ptr_t& address) -> void
    {
        _address = address;
    }

    template <typename T>
    auto MemoryArea<T>::setSize(const size_t& size) -> void
    {
        _size = size;
    }

    template <typename T>
    auto& MemoryArea<T>::protection()
    {
        return _protection;
    }
} // namespace XLib

#endif // MEMORYMAP_H
