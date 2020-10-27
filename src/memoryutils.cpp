#include "memoryutils.h"
#ifndef WINDOWS
    #include <errno.h>
    #include <fstream>
    #include <string.h>
#endif

using namespace XLib;

/**
 * +440    common  rmmap                   sys_rmmap
 * +441    common  rmprotect               sys_rmprotect
 * +442    common  pkey_rmprotect          sys_pkey_rmprotect
 * +443    common  rmunmap                sys_rmunmap
 * +444    common  rclone                  sys_rclone
 */

template <typename T>
constexpr inline auto MemoryUtils::align(T value, safesize_t size)
{
    auto original_value = view_as<uintptr_t>(value);
    original_value -= original_value % size;
    return view_as<T>(original_value);
}

template <typename T>
constexpr inline auto MemoryUtils::align_to_page_size(T sizeToAlign,
                                                      safesize_t pageSize)
{
    return view_as<T>(
      ((view_as<size_t>(sizeToAlign) + (pageSize - 1)) / pageSize)
      * pageSize);
}

template <typename T>
auto MemoryUtils::ProtectMemoryArea(pid_t pid,
                                    T address,
                                    size_t size,
                                    memory_protection_flags_t flags)
  -> void
{
    auto aligned_address = view_as<ptr_t>(align(address, GetPageSize()));
    auto aligned_size    = align_to_page_size(size, GetPageSize());
#ifdef WINDOWS
    auto process_handle = GetCurrentProcessId() == pid ?
                            GetCurrentProcess() :
                            OpenProcess(PROCESS_VM_OPERATION, false, pid);

    if (process_handle == nullptr)
    {
        throw MemoryException(std::string(CURRENT_CONTEXT) + "Couldn't open process");
    }

    DWORD dwOldFlags;
    auto ret = VirtualProtectEx(process_handle,
                                aligned_address,
                                aligned_size,
                                MemoryArea::Protection::toOS(flags),
                                &dwOldFlags);

    if (!ret)
    {
        throw MemoryException(std::string(CURRENT_CONTEXT) + "VirtualProtectEx failed");
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
        throw MemoryException(std::string(CURRENT_CONTEXT) + "System call rmprotect failed");
    }
#endif
}

template <typename T>
auto MemoryUtils::AllocArea(pid_t pid,
                            T address,
                            size_t size,
                            memory_protection_flags_t flags) -> ptr_t
{
#ifdef WINDOWS
    auto process_handle = GetCurrentProcessId() == pid ?
                            GetCurrentProcess() :
                            OpenProcess(PROCESS_VM_OPERATION, false, pid);

    if (process_handle == nullptr)
    {
        throw MemoryException(std::string(CURRENT_CONTEXT) + "Couldn't open process");
    }

    auto area_start = VirtualAllocEx(process_handle,
                                     view_as<ptr_t>(address),
                                     size,
                                     MEM_COMMIT | MEM_RESERVE,
                                     MemoryArea::Protection::toOS(flags));

    CloseHandle(process_handle);

    return view_as<ptr_t>(area_start);
#else
    auto area_start = syscall(440,
                              pid,
                              address,
                              size,
                              MemoryArea::Protection::toOS(flags),
                              MAP_PRIVATE | MAP_ANONYMOUS,
                              0,
                              0);
    return view_as<ptr_t>(area_start);
#endif
}

template <typename T>
auto MemoryUtils::FreeArea(pid_t pid, T address, size_t size) -> void
{
#ifdef WINDOWS
    auto process_handle = GetCurrentProcessId() == pid ?
                            GetCurrentProcess() :
                            OpenProcess(PROCESS_VM_OPERATION, false, pid);

    if (process_handle == nullptr)
    {
        throw MemoryException(std::string(CURRENT_CONTEXT) + "Couldn't open process");
    }

    auto ret = VirtualFreeEx(process_handle,
                             view_as<ptr_t>(address),
                             size,
                             MEM_RELEASE);

    if (!ret)
    {
        throw MemoryException(std::string(CURRENT_CONTEXT) + "VirtualFreeEx failed");
    }

    CloseHandle(process_handle);
#else
    auto ret = syscall(443, pid, address, size);

    if (ret < 0)
    {
        throw MemoryException(std::string(CURRENT_CONTEXT) + "System call rmunmap failed");
    }
#endif
}

#ifdef WINDOWS
static auto _GetPageSize()
{
    SYSTEM_INFO sys_info;

    GetSystemInfo(&sys_info);

    return sys_info.dwPageSize;
}

size_t MemoryUtils::_page_size = _GetPageSize();
#else
size_t MemoryUtils::_page_size = sysconf(_SC_PAGESIZE);
#endif

auto MemoryUtils::GetPageSize() -> size_t
{
    return _page_size;
}

template auto MemoryUtils::ProtectMemoryArea<uintptr_t>(
  pid_t pid,
  uintptr_t address,
  size_t size,
  memory_protection_flags_t flags) -> void;

template auto MemoryUtils::ProtectMemoryArea<ptr_t>(
  pid_t pid,
  ptr_t address,
  size_t size,
  memory_protection_flags_t flags) -> void;

template auto MemoryUtils::AllocArea<uintptr_t>(
  pid_t pid,
  uintptr_t address,
  size_t size,
  memory_protection_flags_t flags) -> ptr_t;

template auto MemoryUtils::AllocArea<ptr_t>(
  pid_t pid,
  ptr_t address,
  size_t size,
  memory_protection_flags_t flags) -> ptr_t;

template auto MemoryUtils::FreeArea<uintptr_t>(pid_t pid,
                                               uintptr_t address,
                                               size_t size = 0) -> void;
template auto MemoryUtils::FreeArea<ptr_t>(pid_t pid,
                                           ptr_t address,
                                           size_t size = 0) -> void;
