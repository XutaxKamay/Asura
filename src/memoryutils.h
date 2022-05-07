#ifndef MEMORYUTILS_H
#define MEMORYUTILS_H

#include "exception.h"
#include "memoryarea.h"
#include "types.h"

#include "custom_linux_syscalls.h"

namespace XKLib
{
    /**
     * Memory utils, mainly used for making compabilities between Linux
     * and Windows API.
     */
    class MemoryUtils
    {
      public:
        static constexpr inline auto Align(const auto value,
                                           const std::size_t size)
        {
            auto original_value = view_as<std::uintptr_t>(value);
            original_value -= original_value % size;
            return view_as<decltype(value)>(original_value);
        }

        static constexpr inline auto AlignToPageSize(
          const auto sizeToAlign,
          const std::size_t pageSize)
        {
            return view_as<decltype(sizeToAlign)>(
              ((view_as<std::size_t>(sizeToAlign) + (pageSize - 1))
               / pageSize)
              * pageSize);
        }

        static auto ProtectMemoryArea(const process_id_t pid,
                                      const auto address,
                                      const std::size_t size,
                                      const mapf_t flags) -> void
        {
            const auto aligned_address = Align<ptr_t>(view_as<ptr_t>(
                                                        address),
                                                      GetPageSize());
            const auto aligned_size    = AlignToPageSize(size,
                                                      GetPageSize());
#ifdef WINDOWS
            const auto process_handle = GetCurrentProcessId() == pid ?
                                          GetCurrentProcess() :
                                          OpenProcess(
                                            PROCESS_VM_OPERATION,
                                            false,
                                            view_as<DWORD>(pid));

            if (process_handle == nullptr)
            {
                XKLIB_EXCEPTION("Couldn't open process");
            }

            DWORD dwOldFlags;
            const auto ret = VirtualProtectEx(
              process_handle,
              aligned_address,
              aligned_size,
              MemoryArea::ProtectionFlags::ToOS(flags),
              &dwOldFlags);

            if (!ret)
            {
                XKLIB_EXCEPTION("VirtualProtectEx failed");
            }

            CloseHandle(process_handle);
#else
            const auto ret = syscall(__NR_rmprotect,
                                     pid,
                                     aligned_address,
                                     aligned_size,
                                     MemoryArea::ProtectionFlags::ToOS(
                                       flags));

            if (ret < 0)
            {
                XKLIB_EXCEPTION("System call rmprotect failed");
            }
#endif
        }

        static auto AllocArea(const process_id_t pid,
                              const auto address,
                              const std::size_t size,
                              const mapf_t flags) -> ptr_t
        {
#ifdef WINDOWS
            const auto process_handle = GetCurrentProcessId() == pid ?
                                          GetCurrentProcess() :
                                          OpenProcess(
                                            PROCESS_VM_OPERATION,
                                            false,
                                            view_as<DWORD>(pid));

            if (process_handle == nullptr)
            {
                XKLIB_EXCEPTION("Couldn't open process");
            }

            const auto ret = VirtualAllocEx(
              process_handle,
              view_as<ptr_t>(address),
              size,
              MEM_COMMIT | MEM_RESERVE,
              MemoryArea::ProtectionFlags::ToOS(flags));

            CloseHandle(process_handle);
#else
            struct
            {
                pid_t pid;
                unsigned long addr;
                unsigned long len;
                unsigned long prot;
                unsigned long flags;
                unsigned long fd;
                unsigned long pgoff;
            } args { pid,
                     view_as<unsigned long>(address),
                     view_as<unsigned long>(size),
                     view_as<unsigned long>(
                       MemoryArea::ProtectionFlags::ToOS(flags)),
                     view_as<unsigned long>(MAP_PRIVATE | MAP_ANONYMOUS),
                     view_as<unsigned long>(-1),
                     0 };

            const auto ret = syscall(__NR_rmmap, &args, sizeof(args));
#endif
            return view_as<ptr_t>(ret);
        }

        static auto FreeArea(const process_id_t pid,
                             const auto address,
                             const std::size_t size) -> void
        {
#ifdef WINDOWS
            const auto process_handle = GetCurrentProcessId() == pid ?
                                          GetCurrentProcess() :
                                          OpenProcess(
                                            PROCESS_VM_OPERATION,
                                            false,
                                            view_as<DWORD>(pid));

            if (process_handle == nullptr)
            {
                XKLIB_EXCEPTION("Couldn't open process");
            }

            const auto ret = VirtualFreeEx(process_handle,
                                           view_as<ptr_t>(address),
                                           size,
                                           MEM_RELEASE);

            if (!ret)
            {
                XKLIB_EXCEPTION("VirtualFreeEx failed");
            }

            CloseHandle(process_handle);
#else
            const auto ret = syscall(__NR_rmunmap, pid, address, size);

            if (ret < 0)
            {
                XKLIB_EXCEPTION(+"System call rmunmap failed");
            }
#endif
        }

        static auto ReadProcessMemoryArea(const process_id_t pid,
                                          const auto address,
                                          const std::size_t size)
          -> bytes_t
        {
            bytes_t result(size);

#ifndef WINDOWS
            const iovec local  = { .iov_base = result.data(),
                                   .iov_len  = result.size() };
            const iovec remote = { .iov_base = view_as<ptr_t>(address),
                                   .iov_len  = result.size() };

            const auto ret = process_vm_readv(pid,
                                              &local,
                                              1,
                                              &remote,
                                              1,
                                              0);

            if (ret != view_as<decltype(ret)>(size))
            {
                XKLIB_EXCEPTION("process_vm_readv failed with "
                                + std::to_string(address)
                                + " and size: " + std::to_string(size));
            }
#else

            const auto ret = Toolhelp32ReadProcessMemory(
              view_as<DWORD>(pid),
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

        template <typename A>
        static auto ReadProcessMemoryAreaAligned(const process_id_t pid,
                                                 const auto address,
                                                 const std::size_t size)
          -> std::vector<A>
        {
            std::vector<A> result(
              AlignToPageSize(size + (sizeof(A) * 2), sizeof(A)));

#ifndef WINDOWS
            const iovec local  = { .iov_base = result.data() + 1,
                                   .iov_len  = result.size() * sizeof(A) };
            const iovec remote = { .iov_base = view_as<ptr_t>(address),
                                   .iov_len = result.size() * sizeof(A) };

            const auto ret = process_vm_readv(pid,
                                              &local,
                                              1,
                                              &remote,
                                              1,
                                              0);

            if (ret != view_as<decltype(ret)>(size * sizeof(A)))
            {
                XKLIB_EXCEPTION("process_vm_readv failed with "
                                + std::to_string(address) + " and size: "
                                + std::to_string(size * sizeof(A)));
            }
#else

            const auto ret = Toolhelp32ReadProcessMemory(
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

        template <typename T = std::uintptr_t>
        static auto WriteProcessMemoryArea(const process_id_t pid,
                                           const bytes_t& bytes,
                                           const T address) -> void
        {
#ifndef WINDOWS
            const iovec local  = { .iov_base = view_as<data_t>(
                                    bytes.data()),
                                   .iov_len = bytes.size() };
            const iovec remote = { .iov_base = view_as<ptr_t>(address),
                                   .iov_len  = bytes.size() };

            const auto ret = process_vm_writev(pid,
                                               &local,
                                               1,
                                               &remote,
                                               1,
                                               0);

            if (ret != view_as<decltype(ret)>(bytes.size()))
            {
                XKLIB_EXCEPTION("process_vm_writev failed");
            }

#else
            const auto process_handle = GetCurrentProcessId() == pid ?
                                          GetCurrentProcess() :
                                          OpenProcess(
                                            PROCESS_VM_OPERATION
                                              | PROCESS_VM_WRITE,
                                            false,
                                            view_as<DWORD>(pid));

            if (process_handle == nullptr)
            {
                XKLIB_EXCEPTION("Couldn't open process");
            }

            const auto ret = WriteProcessMemory(process_handle,
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
