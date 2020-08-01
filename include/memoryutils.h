#ifndef MEMORYUTILS_H
#define MEMORYUTILS_H

#include "memorymap.h"
#include <unordered_map>

namespace XLib
{
    /**
     * @brief MemoryUtils
     * Memory utils, mainly used for making compabilities between Linux
     * and Windows API.
     */
    class MemoryUtils
    {
      public:
        static maps_t queryMaps(pid_t pid);

        template <typename T>
        static auto searchMap(pid_t pid, T address, map_t* pMap = nullptr)
        {
            auto maps = queryMaps(pid);

            for (auto&& map : maps)
            {
                auto start_ptr = view_as<uintptr_t>(map.begin());
                auto end_ptr   = view_as<uintptr_t>(map.end());

                if (view_as<uintptr_t>(address) >= start_ptr
                    && view_as<uintptr_t>(address) < end_ptr)
                {
                    if (pMap)
                        *pMap = map;

                    return true;
                }
            }

            return false;
        }

        static auto protectMap(pid_t pid,
                               map_t& map,
                               map_t::protection_t newFlags);

        template <typename T>
        static auto protectMemory(pid_t pid,
                                  T address,
                                  size_t size,
                                  map_t::protection_t newFlags,
                                  map_t::protection_t* pFlags = nullptr)
        {
        }

        template <typename T>
        static auto allocMap(T address,
                             size_t size,
                             map_t::protection_t newFlags)
        {
        }

        template <typename T>
        static auto freeMap(T address, size_t size)
        {
        }
    };

    template <typename T>
    constexpr inline auto align(T value, safesize_t size)
    {
        auto original_value = view_as<uintptr_t>(value);
        original_value -= original_value % size;
        return view_as<T>(original_value);
    }
}

#endif // MEMORYUTILS_H
