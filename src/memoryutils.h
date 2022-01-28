#ifndef MEMORYUTILS_H
#define MEMORYUTILS_H

#include "custom_linux_syscalls.h"
#include "exception.h"
#include "memoryarea.h"
#include "types.h"

namespace XKLib
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
        static constexpr inline auto Align(T value, std::size_t size)
        {
            auto original_value = view_as<uintptr_t>(value);
            original_value -= original_value % size;
            return view_as<T>(original_value);
        }

        template <typename T = uintptr_t>
        static constexpr inline auto AlignToPageSize(T sizeToAlign,
                                                     std::size_t pageSize)
        {
            return view_as<T>(
              ((view_as<std::size_t>(sizeToAlign) + (pageSize - 1))
               / pageSize)
              * pageSize);
        }

        template <typename T = uintptr_t>
        static auto ProtectMemoryArea(process_id_t pid,
                                      T address,
                                      std::size_t size,
                                      mapf_t flags) -> void
        {
            auto aligned_address = Align<ptr_t>(view_as<ptr_t>(address),
                                                GetPageSize());
            auto aligned_size    = AlignToPageSize(size, GetPageSize());
#ifdef WINDOWS
            auto process_handle = GetCurrentProcessId() == pid ?
                                    GetCurrentProcess() :
                                    OpenProcess(PROCESS_VM_OPERATION,
                                                false,
                                                view_as<DWORD>(pid));

            if (process_handle == nullptr)
            {
                XKLIB_EXCEPTION("Couldn't open process");
            }

            DWORD dwOldFlags;
            auto ret = VirtualProtectEx(process_handle,
                                        aligned_address,
                                        aligned_size,
                                        MemoryArea::ProtectionFlags::ToOS(
                                          flags),
                                        &dwOldFlags);

            if (!ret)
            {
                XKLIB_EXCEPTION("VirtualProtectEx failed");
            }

            CloseHandle(process_handle);
#else
            auto ret = syscall(__NR_rmprotect,
                               pid,
                               aligned_address,
                               aligned_size,
                               MemoryArea::ProtectionFlags::ToOS(flags));

            if (ret < 0)
            {
                XKLIB_EXCEPTION("System call rmprotect failed");
            }
#endif
        }

        template <typename T = uintptr_t>
        static auto AllocArea(process_id_t pid,
                              T address,
                              std::size_t size,
                              mapf_t flags) -> ptr_t
        {
#ifdef WINDOWS
            auto process_handle = GetCurrentProcessId() == pid ?
                                    GetCurrentProcess() :
                                    OpenProcess(PROCESS_VM_OPERATION,
                                                false,
                                                view_as<DWORD>(pid));

            if (process_handle == nullptr)
            {
                XKLIB_EXCEPTION("Couldn't open process");
            }

            auto area_start = VirtualAllocEx(
              process_handle,
              view_as<ptr_t>(address),
              size,
              MEM_COMMIT | MEM_RESERVE,
              MemoryArea::ProtectionFlags::ToOS(flags));

            CloseHandle(process_handle);

            return view_as<ptr_t>(area_start);
#else
            auto area_start = syscall(__NR_rmmap,
                                      pid,
                                      address,
                                      size,
                                      MemoryArea::ProtectionFlags::ToOS(
                                        flags),
                                      MAP_PRIVATE | MAP_ANONYMOUS,
                                      0,
                                      0);
            return view_as<ptr_t>(area_start);
#endif
        }

        template <typename T = uintptr_t>
        static auto FreeArea(process_id_t pid, T address, std::size_t size)
          -> void
        {
#ifdef WINDOWS
            auto process_handle = GetCurrentProcessId() == pid ?
                                    GetCurrentProcess() :
                                    OpenProcess(PROCESS_VM_OPERATION,
                                                false,
                                                view_as<DWORD>(pid));

            if (process_handle == nullptr)
            {
                XKLIB_EXCEPTION("Couldn't open process");
            }

            auto ret = VirtualFreeEx(process_handle,
                                     view_as<ptr_t>(address),
                                     size,
                                     MEM_RELEASE);

            if (!ret)
            {
                XKLIB_EXCEPTION("VirtualFreeEx failed");
            }

            CloseHandle(process_handle);
#else
            auto ret = syscall(__NR_rmunmap, pid, address, size);

            if (ret < 0)
            {
                XKLIB_EXCEPTION(+"System call rmunmap failed");
            }
#endif
        }

        template <typename T = uintptr_t>
        static auto ReadProcessMemoryArea(process_id_t pid,
                                          T address,
                                          std::size_t size) -> bytes_t
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
                XKLIB_EXCEPTION("process_vm_readv failed with "
                                + std::to_string(address)
                                + " and size: " + std::to_string(size));
            }
#else

            auto ret = Toolhelp32ReadProcessMemory(view_as<DWORD>(pid),
                                                   view_as<ptr_t>(address),
                                                   result.data(),
                                                   result.size(),
                                                   nullptr);

            if (!ret)
            {
                XKLIB_EXCEPTION("ReadProcessMemory failed with "
                                + std::to_string(address)
                                + " and size: " + std::to_string(size));
            }
#endif

            return result;
        }

        template <typename A, typename T = uintptr_t>
        static auto ReadProcessMemoryAreaAligned(process_id_t pid,
                                                 T address,
                                                 std::size_t size)
          -> std::vector<A>
        {
            std::vector<A> result(AlignToPageSize(size, sizeof(A)));

#ifndef WINDOWS
            iovec local  = { .iov_base = result.data(),
                             .iov_len  = result.size() * sizeof(A) };
            iovec remote = { .iov_base = view_as<ptr_t>(address),
                             .iov_len  = result.size() * sizeof(A) };

            auto ret = process_vm_readv(pid, &local, 1, &remote, 1, 0);

            if (ret != view_as<decltype(ret)>(size * sizeof(A)))
            {
                XKLIB_EXCEPTION("process_vm_readv failed with "
                                + std::to_string(address) + " and size: "
                                + std::to_string(size * sizeof(A)));
            }
#else

            auto ret = Toolhelp32ReadProcessMemory(
              view_as<DWORD>(pid),
              view_as<ptr_t>(address),
              result.data(),
              result.size() * sizeof(A),
              nullptr);

            if (!ret)
            {
                XKLIB_EXCEPTION("ReadProcessMemory failed with "
                                + std::to_string(address)
                                + " and size: " + std::to_string(size));
            }
#endif

            return result;
        }

        template <typename T = uintptr_t>
        static auto WriteProcessMemoryArea(process_id_t pid,
                                           const bytes_t& bytes,
                                           T address) -> void
        {
#ifndef WINDOWS
            iovec local  = { .iov_base = view_as<data_t>(bytes.data()),
                             .iov_len  = bytes.size() };
            iovec remote = { .iov_base = view_as<ptr_t>(address),
                             .iov_len  = bytes.size() };

            auto ret = process_vm_writev(pid, &local, 1, &remote, 1, 0);

            if (ret != view_as<decltype(ret)>(bytes.size()))
            {
                XKLIB_EXCEPTION("process_vm_writev failed");
            }

#else
            auto process_handle = GetCurrentProcessId() == pid ?
                                    GetCurrentProcess() :
                                    OpenProcess(PROCESS_VM_OPERATION
                                                  | PROCESS_VM_WRITE,
                                                false,
                                                view_as<DWORD>(pid));

            if (process_handle == nullptr)
            {
                XKLIB_EXCEPTION("Couldn't open process");
            }

            auto ret = WriteProcessMemory(process_handle,
                                          view_as<ptr_t>(address),
                                          bytes.data(),
                                          bytes.size(),
                                          nullptr);

            if (!ret)
            {
                XKLIB_EXCEPTION("WriteProcessMemory failed");
            }

            CloseHandle(process_handle);
#endif
        }

        static auto GetPageSize() -> std::size_t;

      private:
        static std::size_t _page_size;
        static std::once_flag _get_page_size_once_flag;
    };

} // namespace XKLib

#endif // MEMORYUTILS_H
