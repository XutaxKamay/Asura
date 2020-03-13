#ifdef WINDOWS
    #include <windows.h>
#else
    #include <sys/mman.h>
#endif
#include "memoryutils.h"
#include <unistd.h>

using namespace XLib;

static auto g_pageSize = sysconf( _SC_PAGESIZE );

/* TODO: replace by syscalls for being stealth */
auto protect( pid_t pid,
              ptr_t address,
              size_t size,
              map_t::protection_t newFlags,
              map_t::protection_t* pFlags = nullptr )
{
#ifdef WINDOWS
    /* TODO: Find a proper way to get stealth handles for remote pids */
    if ( pid == GetCurrentProcessId() )
    {
        DWORD dwOld;

        if ( VirtualProtect( address, size, newFlags, &dwOld ) )
        {
            if ( pFlags )
                *pFlags = dwOld;

            return true;
        }
    }
    else
    {
        ConsoleOutput( "Cannot VirtualProtect from a remote pid yet..." )
          << std::endl;
    }
#else
    auto alignedSize = ( ( size + g_pageSize ) / g_pageSize )
                       * g_pageSize;

    auto alignedAddress = align( address, g_pageSize );

    /* We might need a kernel module for ioctl */
    if ( getpid() == pid )
    {
        auto ret = mprotect( address, alignedSize, newFlags );

        if ( ret != -1 )
        {
            return true;
        }
    }
    else
    {
        ConsoleOutput( "Cannot mprotect from a remote pid yet..." )
          << std::endl;
    }
#endif

    return false;
}

maps_t XLib::MemoryUtils::queryMaps( pid_t pid )
{
    maps_t maps;

    /* Let's do the magic here */

    return maps;
}
