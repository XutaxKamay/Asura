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
        static auto QueryMap(pid_t pid) -> memory_map_t;

        template <typename T>
        static auto SearchArea(pid_t pid,
                               T address,
                               memory_area_t* pArea = nullptr)
        {
            auto memory_map = QueryMap(pid);

            for (auto&& area : memory_map)
            {
                auto start_ptr = view_as<uintptr_t>(area.begin());
                auto end_ptr   = view_as<uintptr_t>(area.end());

                if (view_as<uintptr_t>(address) >= start_ptr
                    && view_as<uintptr_t>(address) < end_ptr)
                {
                    if (pArea)
                        *pArea = area;

                    return true;
                }
            }

            return false;
        }

        static auto ProtectArea(
          pid_t pid,
          memory_area_t& area,
          memory_area_t::protection_t newFlags,
          memory_area_t::protection_t* pFlags = nullptr) -> void;

        template <typename T>
        static auto ProtectMemory(
          pid_t pid,
          T address,
          size_t size,
          memory_area_t::protection_t newFlags,
          memory_area_t::protection_t* pFlags = nullptr) -> void
        {
            memory_area_t area;
            auto aligned_address = view_as<ptr_t>(
              align(address, GetPageSize()));
            auto aligned_size = align_to_page_size(size, GetPageSize());

            if (!SearchArea(pid, aligned_address, &area))
            {
                throw MemoryException("Could not find area in mapped "
                                      "memory");
            }

            if (pFlags)
            {
                *pFlags = area.protection();
            }

#ifdef WINDOWS
            auto process_handle = GetCurrentProcessId() == pid ?
                                    GetCurrentProcess() :
                                    OpenProcess(PROCESS_VM_OPERATION,
                                                false,
                                                pid);

            if (process_handle == nullptr)
            {
                throw MemoryException("Couldn't open process");
            }

            DWORD dwOldFlags;
            auto ret = VirtualProtectEx(process_handle,
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

            CloseHandle(process_handle);
#else
            auto ret = syscall(441,
                               pid,
                               aligned_address,
                               aligned_size,
                               ConvertOwnProtToOS(newFlags));

            if (ret < 0)
            {
                throw MemoryException("System call rmprotect failed");
            }
#endif
        }

        template <typename T>
        static auto AllocArea(pid_t pid,
                              T address,
                              size_t size,
                              memory_area_t::protection_t newFlags)
        {
#ifdef WINDOWS
            auto process_handle = GetCurrentProcessId() == pid ?
                                    GetCurrentProcess() :
                                    OpenProcess(PROCESS_VM_OPERATION,
                                                false,
                                                pid);

            if (process_handle == nullptr)
            {
                throw MemoryException("Couldn't open process");
            }

            auto area_start = VirtualAllocEx(process_handle,
                                             address,
                                             size,
                                             MEM_COMMIT | MEM_RESERVE,
                                             ConvertOwnProtToOS(newFlags));

            CloseHandle(process_handle);

            return area_start;
#else
            return syscall(440,
                           pid,
                           address,
                           size,
                           ConvertOwnProtToOS(newFlags),
                           MAP_PRIVATE | MAP_ANONYMOUS,
                           0,
                           0);
#endif
        }

        template <typename T>
        static auto FreeArea(pid_t pid, T address, size_t size = 0)
          -> void
        {
#ifdef WINDOWS
            auto process_handle = GetCurrentProcessId() == pid ?
                                    GetCurrentProcess() :
                                    OpenProcess(PROCESS_VM_OPERATION,
                                                false,
                                                pid);

            if (process_handle == nullptr)
            {
                throw MemoryException("Couldn't open process");
            }

            auto ret = VirtualFreeEx(process_handle,
                                     address,
                                     size,
                                     MEM_RELEASE);

            if (!ret)
            {
                throw MemoryException("VirtualFreeEx failed");
            }

            CloseHandle(process_handle);

#else
            auto ret = syscall(443, pid, address, size);

            if (ret < 0)
            {
                throw MemoryException("System call rmunmap failed");
            }
#endif
        }

        static auto FreeArea(pid_t pid, memory_area_t area) -> void;

        static auto GetPageSize() -> size_t;

        static auto ConvertOSProtToOwn(int flags)
          -> memory_area_t::protection_t;
        static auto ConvertOwnProtToOS(memory_area_t::protection_t flags)
          -> int;

      private:
        static size_t _page_size;
    };

} // namespace XLib

#endif // MEMORYUTILS_H
