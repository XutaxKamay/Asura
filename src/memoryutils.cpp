#include "memoryutils.h"
#ifdef WINDOWS
    #include <windows.h>
#else
    #include <sys/file.h>
    #include <sys/ioctl.h>
    #include <sys/mman.h>
#endif
#include <unistd.h>

using namespace XLib;

/**
 * auto alignedSize = ( ( size + g_pageSize ) / g_pageSize )
 *                 * g_pageSize;
 *
 * auto alignedAddress = align( address, g_pageSize );
 */

maps_t MemoryUtils::queryMaps(pid_t)
{
    maps_t maps;

#ifndef WINDOWS

#else

#endif

    return maps;
}
