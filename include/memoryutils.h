#ifndef MEMORYUTILS_H
#define MEMORYUTILS_H

#include "memoryarea.h"
#include "memoryexception.h"
#include "types.h"

#ifndef WINDOWS
    #include <sys/file.h>
    #include <sys/ioctl.h>
    #include <sys/mman.h>
    #include <sys/types.h>
    #include <unistd.h>
#else
    #include <windows.h>
#endif

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
        template <typename T>
        static constexpr inline auto align(T value, safesize_t size)
        {
            auto original_value = view_as<uintptr_t>(value);
            original_value -= original_value % size;
            return view_as<T>(original_value);
        }

        template <typename T>
        static constexpr inline auto align_to_page_size(
          T sizeToAlign,
          safesize_t pageSize)
        {
            return view_as<T>(
              ((view_as<size_t>(sizeToAlign) + (pageSize - 1)) / pageSize)
              * pageSize);
        }

        template <typename T = uintptr_t>
        static auto ProtectMemoryArea(pid_t pid,
                                  T address,
                                  size_t size,
                                  memory_protection_flags_t flags) -> void
        {
            auto aligned_address = view_as<ptr_t>(
              align(address, GetPageSize()));
            auto aligned_size = align_to_page_size(size, GetPageSize());
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
                                        MemoryArea::Protection::toOS(
                                          flags),
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
                               MemoryArea::Protection::toOS(flags));

            if (ret < 0)
            {
                throw MemoryException("System call rmprotect failed");
            }
#endif
        }

        template <typename T = uintptr_t>
        static auto AllocArea(pid_t pid,
                              T address,
                              size_t size,
                              memory_protection_flags_t flags)
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
                                             view_as<ptr_t>(address),
                                             size,
                                             MEM_COMMIT | MEM_RESERVE,
                                             MemoryArea::Protection::toOS(
                                               flags));

            CloseHandle(process_handle);

            return area_start;
#else
            return syscall(440,
                           pid,
                           address,
                           size,
                           MemoryArea::Protection::toOS(flags),
                           MAP_PRIVATE | MAP_ANONYMOUS,
                           0,
                           0);
#endif
        }

        template <typename T = uintptr_t>
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
                                     view_as<ptr_t>(address),
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

        static auto GetPageSize() -> size_t;

      private:
        static size_t _page_size;
    };

} // namespace XLib

#endif // MEMORYUTILS_H
