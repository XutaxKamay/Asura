#include "memoryutils.h"
#ifndef WINDOWS
    #include <fstream>
#endif

/**
 * +440    common  rmarea                   sys_rmarea
 * +441    common  rmprotect               sys_rmprotect
 * +442    common  pkey_rmprotect          sys_pkey_rmprotect
 * +443    common  rmunarea                 sys_rmunarea
 * +444    common  rclone                  sys_rclone
 */

using namespace XLib;

/**
 * auto alignedSize = ( ( size + g_pageSize ) / g_pageSize )
 *                 * g_pageSize;
 *
 * auto alignedAddress = align( address, g_pageSize );
 */

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

auto MemoryUtils::QueryMap(pid_t pid) -> memory_map_t
{
    memory_map_t memory_map;

#ifndef WINDOWS
    std::ifstream file_memory_map("/proc/" + std::to_string(pid)
                                  + "/memory_map");
    std::string line;

    if (!file_memory_map.is_open())
    {
        throw MemoryException("Couldn't open /proc/" + std::to_string(pid)
                              + "/memory_map");
    }

    while (std::getline(file_memory_map, line))
    {
        memory_area_t area;
        uintptr_t start, end;
        byte_t prot[3];

        /* 0x0 0x1000 rwx */
        sscanf(line.c_str(),
               "%p-%p %c%c%c",
               (ptr_t*)&start,
               (ptr_t*)&end,
               &prot[0],
               &prot[1],
               &prot[2]);

        auto is_on = [](byte_t prot)
        {
            if (prot == '-')
            {
                return false;
            }

            return true;
        };

        area.protection() = view_as<memory_area_t::protection_t>(
          (is_on(prot[0]) ? memory_area_t::protection_t::READ : 0)
          | (is_on(prot[1]) ? memory_area_t::protection_t::WRITE : 0)
          | (is_on(prot[2]) ? memory_area_t::protection_t::EXECUTE : 0));

        area.setAddress(view_as<ptr_t>(start));
        area.setSize(end - start);
        memory_map.push_back(area);
    }

#else
    auto process_handle = OpenProcess(PROCESS_QUERY_INFORMATION,
                                      false,
                                      pid);

    if (process_handle == nullptr)
    {
        throw MemoryException("Couldn't open process from pid: "
                              + std::to_string(pid));
    }

    MEMORY_BASIC_INFORMATION info;
    data_t bs;

    for (bs = nullptr;
         VirtualQueryEx(process_handle, bs, &info, sizeof(info))
         == sizeof(info);
         bs += info.RegionSize)
    {
        /**
         * We care only about virtual memory that has physical memory.
         */
        if (info.State != MEM_COMMIT)
        {
            continue;
        }

        memory_area_t area;
        area.setAddress(bs);
        area.setSize(info.RegionSize);
        area.protection() = ConvertOSProtToOwn(info.Protect);

        memory_map.push_back(area);
    }

    CloseHandle(process_handle);
#endif

    return memory_map;
}

auto MemoryUtils::ProtectArea(pid_t pid,
                              memory_area_t& area,
                              memory_area_t::protection_t newFlags,
                              memory_area_t::protection_t* pFlags) -> void
{
    try
    {
        ProtectMemory(pid,
                      area.begin(),
                      view_as<uintptr_t>(area.end())
                        - view_as<uintptr_t>(area.begin()),
                      newFlags,
                      pFlags);
    }
    catch (MemoryException& me)
    {
        throw me;
    }

    area.protection() = newFlags;
}

auto MemoryUtils::GetPageSize() -> size_t
{
    return _page_size;
}

auto MemoryUtils::ConvertOSProtToOwn(int flags)
  -> memory_area_t::protection_t
{
#ifdef WINDOWS
    memory_area_t::protection_t own_flags;
    switch (flags)
    {
        case PAGE_EXECUTE:
        {
            own_flags = view_as<memory_area_t::protection_t>(
              memory_area_t::protection_t::EXECUTE);
            break;
        }
        case PAGE_EXECUTE_READ:
        {
            own_flags = view_as<memory_area_t::protection_t>(
              memory_area_t::protection_t::EXECUTE
              | memory_area_t::protection_t::READ);
            break;
        }
        case PAGE_EXECUTE_READWRITE:
        {
            own_flags = view_as<memory_area_t::protection_t>(
              memory_area_t::protection_t::EXECUTE
              | memory_area_t::protection_t::READ
              | memory_area_t::protection_t::WRITE);
            break;
        }
        case PAGE_READONLY:
        {
            own_flags = view_as<memory_area_t::protection_t>(
              memory_area_t::protection_t::READ);
            break;
        }
        case PAGE_READWRITE:
        {
            own_flags = view_as<memory_area_t::protection_t>(
              memory_area_t::protection_t::READ
              | memory_area_t::protection_t::WRITE);
            break;
        }
        case PAGE_EXECUTE_WRITECOPY:
        {
            own_flags = view_as<memory_area_t::protection_t>(
              memory_area_t::protection_t::EXECUTE
              | memory_area_t::protection_t::WRITE);
            break;
        }
        case PAGE_WRITECOPY:
        {
            own_flags = view_as<memory_area_t::protection_t>(
              memory_area_t::protection_t::WRITE);
            break;
        }
        default:
        {
            own_flags = memory_area_t::protection_t::NONE;
            break;
        }
    }

    return own_flags;

#else
    return view_as<memory_area_t::protection_t>(flags);
#endif
}

auto MemoryUtils::ConvertOwnProtToOS(memory_area_t::protection_t flags)
  -> int
{
#ifdef WINDOWS
    int os_flags;

    switch (view_as<int>(flags))
    {
        case memory_area_t::protection_t::EXECUTE:
        {
            os_flags = PAGE_EXECUTE;
            break;
        }
        case memory_area_t::protection_t::EXECUTE
          | memory_area_t::protection_t::READ:
        {
            os_flags = PAGE_EXECUTE_READ;
            break;
        }
        case memory_area_t::protection_t::EXECUTE
          | memory_area_t::protection_t::READ
          | memory_area_t::protection_t::WRITE:
        {
            os_flags = PAGE_EXECUTE_READWRITE;
            break;
        }
        case memory_area_t::protection_t::READ:
        {
            os_flags = PAGE_READONLY;
            break;
        }
        case memory_area_t::protection_t::READ
          | memory_area_t::protection_t::WRITE:
        {
            os_flags = PAGE_READWRITE;
            break;
        }
        case memory_area_t::protection_t::EXECUTE
          | memory_area_t::protection_t::WRITE:
        {
            os_flags = PAGE_EXECUTE_WRITECOPY;
            break;
        }
        case memory_area_t::protection_t::WRITE:
        {
            os_flags = PAGE_WRITECOPY;
            break;
        }
        default:
        {
            os_flags = 0;
            break;
        }
    }

    return os_flags;
#else
    return flags;
#endif
}

auto MemoryUtils::FreeArea(pid_t pid, memory_area_t area) -> void
{
    FreeArea(pid,
             area.begin(),
             view_as<uintptr_t>(area.end())
               - view_as<uintptr_t>(area.begin()));
}
