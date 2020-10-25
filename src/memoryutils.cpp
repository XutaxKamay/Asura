#include "memoryutils.h"
#ifndef WINDOWS
    #include <fstream>
#endif

/**
 * +440    common  rmmap                   sys_rmmap
 * +441    common  rmprotect               sys_rmprotect
 * +442    common  pkey_rmprotect          sys_pkey_rmprotect
 * +443    common  rmunmap                 sys_rmunmap
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
    SYSTEM_INFO sysInfo;

    GetSystemInfo(&sysInfo);

    return sysInfo.dwPageSize;
}

size_t MemoryUtils::_page_size = _GetPageSize();
#else
size_t MemoryUtils::_page_size = sysconf(_SC_PAGESIZE);
#endif

maps_t MemoryUtils::QueryMaps(pid_t pid)
{
    maps_t maps;

#ifndef WINDOWS
    std::ifstream file_maps("/proc/" + std::to_string(pid) + "/maps");
    std::string line;

    if (!file_maps.is_open())
    {
        throw MemoryException("Couldn't open /proc/" + std::to_string(pid)
                              + "/maps");
    }

    while (std::getline(file_maps, line))
    {
        map_t map;
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

        map.protection() = view_as<map_t::protection_t>(
          (is_on(prot[0]) ? map_t::protection_t::READ : 0)
          | (is_on(prot[1]) ? map_t::protection_t::WRITE : 0)
          | (is_on(prot[2]) ? map_t::protection_t::EXECUTE : 0));

        map.setAddress(view_as<ptr_t>(start));
        map.setSize(end - start);
        maps.push_back(map);
    }

#else
    auto handle_process = OpenProcess(PROCESS_QUERY_INFORMATION,
                                      false,
                                      pid);

    if (handle_process == nullptr)
    {
        throw MemoryException("Couldn't open process from pid: "
                              + std::to_string(pid));
    }

    MEMORY_BASIC_INFORMATION info;
    data_t bs;

    for (bs = nullptr;
         VirtualQueryEx(handle_process, bs, &info, sizeof(info))
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

        map_t map;
        map.setAddress(bs);
        map.setSize(info.RegionSize);
        map.protection() = ConvertOSProtToOwn(info.Protect);

        maps.push_back(map);
    }

    CloseHandle(handle_process);
#endif

    return maps;
}

void MemoryUtils::ProtectMap(pid_t pid,
                             map_t& map,
                             map_t::protection_t newFlags,
                             map_t::protection_t* pFlags)
{
    try
    {
        ProtectMemory(pid,
                      map.begin(),
                      view_as<uintptr_t>(map.end())
                        - view_as<uintptr_t>(map.begin()),
                      newFlags,
                      pFlags);
    }
    catch (MemoryException& me)
    {
        throw me;
    }

    map.protection() = newFlags;
}

size_t MemoryUtils::GetPageSize()
{
    return _page_size;
}

map_t::protection_t MemoryUtils::ConvertOSProtToOwn(int flags)
{
#ifdef WINDOWS
    map_t::protection_t own_flags;
    switch (flags)
    {
        case PAGE_EXECUTE:
        {
            own_flags = view_as<map_t::protection_t>(
              map_t::protection_t::EXECUTE);
            break;
        }
        case PAGE_EXECUTE_READ:
        {
            own_flags = view_as<map_t::protection_t>(
              map_t::protection_t::EXECUTE | map_t::protection_t::READ);
            break;
        }
        case PAGE_EXECUTE_READWRITE:
        {
            own_flags = view_as<map_t::protection_t>(
              map_t::protection_t::EXECUTE | map_t::protection_t::READ
              | map_t::protection_t::WRITE);
            break;
        }
        case PAGE_READONLY:
        {
            own_flags = view_as<map_t::protection_t>(
              map_t::protection_t::READ);
            break;
        }
        case PAGE_READWRITE:
        {
            own_flags = view_as<map_t::protection_t>(
              map_t::protection_t::READ | map_t::protection_t::WRITE);
            break;
        }
        case PAGE_EXECUTE_WRITECOPY:
        {
            own_flags = view_as<map_t::protection_t>(
              map_t::protection_t::EXECUTE | map_t::protection_t::WRITE);
            break;
        }
        case PAGE_WRITECOPY:
        {
            own_flags = view_as<map_t::protection_t>(
              map_t::protection_t::WRITE);
            break;
        }
        default:
        {
            own_flags = map_t::protection_t::NONE;
            break;
        }
    }

    return own_flags;

#else
    return view_as<map_t::protection_t>(flags);
#endif
}

int MemoryUtils::ConvertOwnProtToOS(map_t::protection_t flags)
{
    return flags;
}
