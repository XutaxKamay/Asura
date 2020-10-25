#ifndef MEMORYUTILS_H
#define MEMORYUTILS_H

#include "memoryexception.h"
#include "memorymap.h"

#ifndef WINDOWS
    #include <sys/file.h>
    #include <sys/ioctl.h>
    #include <sys/mman.h>
    #include <sys/types.h>
    #include <unistd.h>
#else
    #include <windows.h>
#endif

#include <exception>
#include <unordered_map>

namespace XLib
{
    template <typename T>
    constexpr inline auto align(T value, safesize_t size)
    {
        auto original_value = view_as<uintptr_t>(value);
        original_value -= original_value % size;
        return view_as<T>(original_value);
    }

    template <typename T>
    constexpr inline auto align_to_page_size(T sizeToAlign,
                                             safesize_t pageSize)
    {
        return view_as<T>(((sizeToAlign + (pageSize - 1)) / pageSize)
                          * pageSize);
    }

    /**
     * @brief MemoryUtils
     * Memory utils, mainly used for making compabilities between Linux
     * and Windows API.
     */
    class MemoryUtils
    {
      public:
        static maps_t QueryMaps(pid_t pid);

        template <typename T>
        static auto SearchMap(pid_t pid, T address, map_t* pMap = nullptr)
        {
            auto maps = QueryMaps(pid);

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

        static void ProtectMap(pid_t pid,
                               map_t& map,
                               map_t::protection_t newFlags,
                               map_t::protection_t* pFlags = nullptr);

        template <typename T>
        static void ProtectMemory(pid_t pid,
                                  T address,
                                  size_t size,
                                  map_t::protection_t newFlags,
                                  map_t::protection_t* pFlags = nullptr)
        {
            map_t map;
            auto aligned_address = view_as<ptr_t>(
              align(address, GetPageSize()));
            auto aligned_size = align_to_page_size(size, GetPageSize());

            if (!SearchMap(pid, aligned_address, &map))
            {
                throw MemoryException("Could not find mapped memory");
            }

            if (pFlags)
            {
                *pFlags = map.protection();
            }

#ifdef WINDOWS
            auto handle_process = GetCurrentProcessId() == pid ?
                                    GetCurrentProcess() :
                                    OpenProcess(PROCESS_VM_OPERATION,
                                                false,
                                                pid);

            if (handle_process == nullptr)
            {
                throw MemoryException("Couldn't open process");
            }

            DWORD dwOldFlags;
            auto ret = VirtualProtectEx(handle_process,
                                        aligned_address,
                                        aligned_size,
                                        ConvertOwnProtToOS(newFlags),
                                        &dwOldFlags);

            if (!ret)
            {
                throw MemoryException("VirtualProtectEx failed");
            }

            if (pFlags)
            {
                *pFlags = ConvertOSProtToOwn(dwOldFlags);
            }

            CloseHandle(handle_process);
#else
            /**
             * linux-xkmod kernel
             */

            auto ret = syscall(441,
                               pid,
                               aligned_address,
                               aligned_size,
                               newFlags);

            if (ret < 0)
            {
                throw MemoryException("System call rmprotect failed");
            }
#endif
        }

        template <typename T>
        static auto AllocMap(pid_t pid,
                             T address,
                             size_t size,
                             map_t::protection_t newFlags)
        {
        }

        template <typename T>
        static auto FreeMap(pid_t pid, T address, size_t size);

        static auto FreeMap(pid_t pid, map_t map, size_t size);

        static size_t GetPageSize();

        static map_t::protection_t ConvertOSProtToOwn(int flags);
        static int ConvertOwnProtToOS(map_t::protection_t flags);

      private:
        static size_t _page_size;
    };

} // namespace XLib

#endif // MEMORYUTILS_H
