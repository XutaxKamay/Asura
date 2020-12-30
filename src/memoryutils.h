#ifndef MEMORYUTILS_H
#define MEMORYUTILS_H

#include "custom_linux_syscalls.h"
#include "memoryarea.h"
#include "memoryexception.h"
#include "types.h"

#ifndef WINDOWS
    #include <fstream>

    #include <errno.h>
    #include <string.h>
    #include <sys/file.h>
    #include <sys/ioctl.h>
    #include <sys/mman.h>
    #include <sys/types.h>
    #include <sys/uio.h>
    #include <unistd.h>
#else
    #include <windows.h>
#endif

#ifndef WINDOWS

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
        template <typename T = uintptr_t>
        static constexpr inline auto align(T value, safesize_t size)
        {
            auto original_value = view_as<uintptr_t>(value);
            original_value -= original_value % size;
            return view_as<T>(original_value);
        }

        template <typename T = uintptr_t>
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
                                      mapf_t flags) -> void
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
                throw MemoryException(std::string(CURRENT_CONTEXT)
                                      + "Couldn't open process");
            }

            DWORD dwOldFlags;
            auto ret = VirtualProtectEx(process_handle,
                                        aligned_address,
                                        aligned_size,
                                        MemoryArea::ProtectionFlags::toOS(
                                          flags),
                                        &dwOldFlags);

            if (!ret)
            {
                throw MemoryException(std::string(CURRENT_CONTEXT)
                                      + "VirtualProtectEx failed");
            }

            CloseHandle(process_handle);
#else
            auto ret = syscall(__NR_rmprotect,
                               pid,
                               aligned_address,
                               aligned_size,
                               MemoryArea::ProtectionFlags::toOS(flags));

            if (ret < 0)
            {
                throw MemoryException(std::string(CURRENT_CONTEXT)
                                      + "System call rmprotect failed");
            }
#endif
        }

        template <typename T = uintptr_t>
        static auto AllocArea(pid_t pid,
                              T address,
                              size_t size,
                              mapf_t flags) -> ptr_t
        {
#ifdef WINDOWS
            auto process_handle = GetCurrentProcessId() == pid ?
                                    GetCurrentProcess() :
                                    OpenProcess(PROCESS_VM_OPERATION,
                                                false,
                                                pid);

            if (process_handle == nullptr)
            {
                throw MemoryException(std::string(CURRENT_CONTEXT)
                                      + "Couldn't open process");
            }

            auto area_start = VirtualAllocEx(
              process_handle,
              view_as<ptr_t>(address),
              size,
              MEM_COMMIT | MEM_RESERVE,
              MemoryArea::ProtectionFlags::toOS(flags));

            CloseHandle(process_handle);

            return view_as<ptr_t>(area_start);
#else
            auto area_start = syscall(__NR_rmmap,
                                      pid,
                                      address,
                                      size,
                                      MemoryArea::ProtectionFlags::toOS(
                                        flags),
                                      MAP_PRIVATE | MAP_ANONYMOUS,
                                      0,
                                      0);
            return view_as<ptr_t>(area_start);
#endif
        }

        template <typename T = uintptr_t>
        static auto FreeArea(pid_t pid, T address, size_t size) -> void
        {
#ifdef WINDOWS
            auto process_handle = GetCurrentProcessId() == pid ?
                                    GetCurrentProcess() :
                                    OpenProcess(PROCESS_VM_OPERATION,
                                                false,
                                                pid);

            if (process_handle == nullptr)
            {
                throw MemoryException(std::string(CURRENT_CONTEXT)
                                      + "Couldn't open process");
            }

            auto ret = VirtualFreeEx(process_handle,
                                     view_as<ptr_t>(address),
                                     size,
                                     MEM_RELEASE);

            if (!ret)
            {
                throw MemoryException(std::string(CURRENT_CONTEXT)
                                      + "VirtualFreeEx failed");
            }

            CloseHandle(process_handle);
#else
            auto ret = syscall(__NR_rmunmap, pid, address, size);

            if (ret < 0)
            {
                throw MemoryException(std::string(CURRENT_CONTEXT)
                                      + "System call rmunmap failed");
            }
#endif
        }

        template <typename T = uintptr_t>
        static auto ReadProcessMemoryArea(pid_t pid,
                                          T address,
                                          size_t size) -> bytes_t
        {
            bytes_t result(size);

#ifndef WINDOWS
            iovec local  = { .iov_base = result.data(),
                            .iov_len  = result.size() };
            iovec remote = { .iov_base = view_as<ptr_t>(address),
                             .iov_len  = result.size() };

            auto ret = process_vm_readv(pid, &local, 1, &remote, 1, 0);

            if (ret != view_as<decltype(ret)>(size))
            {
                throw MemoryException(std::string(CURRENT_CONTEXT)
                                      + "process_vm_readv failed");
            }
#else
            auto process_handle = GetCurrentProcessId() == pid ?
                                    GetCurrentProcess() :
                                    OpenProcess(PROCESS_VM_OPERATION,
                                                false,
                                                pid);

            if (process_handle == nullptr)
            {
                throw MemoryException(std::string(CURRENT_CONTEXT)
                                      + "Couldn't open process");
            }

            auto ret = ReadProcessMemory(process_handle,
                                         view_as<ptr_t>(address),
                                         result.data(),
                                         result.size(),
                                         nullptr);

            if (!ret)
            {
                throw MemoryException(std::string(CURRENT_CONTEXT)
                                      + "ReadProcessMemory failed");
            }

            CloseHandle(process_handle);
#endif

            return result;
        }

        template <typename T = uintptr_t>
        static auto WriteProcessMemoryArea(pid_t pid,
                                           bytes_t bytes,
                                           T address) -> void
        {
#ifndef WINDOWS
            iovec local  = { .iov_base = bytes.data(),
                            .iov_len  = bytes.size() };
            iovec remote = { .iov_base = view_as<ptr_t>(address),
                             .iov_len  = bytes.size() };

            auto ret = process_vm_writev(pid, &local, 1, &remote, 1, 0);

            if (ret != view_as<decltype(ret)>(bytes.size()))
            {
                throw MemoryException(std::string(CURRENT_CONTEXT)
                                      + "process_vm_writev failed");
            }

#else
            auto process_handle = GetCurrentProcessId() == pid ?
                                    GetCurrentProcess() :
                                    OpenProcess(PROCESS_VM_OPERATION,
                                                false,
                                                pid);

            if (process_handle == nullptr)
            {
                throw MemoryException(std::string(CURRENT_CONTEXT)
                                      + "Couldn't open process");
            }

            auto ret = WriteProcessMemory(process_handle,
                                          view_as<ptr_t>(address),
                                          bytes.data(),
                                          bytes.size(),
                                          nullptr);

            if (!ret)
            {
                throw MemoryException(std::string(CURRENT_CONTEXT)
                                      + "WriteProcessMemory failed");
            }

            CloseHandle(process_handle);
#endif
        }

        static auto GetPageSize() -> size_t;

      private:
        static size_t _page_size;
    };

} // namespace XLib

#endif // MEMORYUTILS_H
