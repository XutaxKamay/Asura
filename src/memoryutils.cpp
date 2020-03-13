#include "memoryutils.h"
#ifdef WINDOWS
    #include <windows.h>
#endif
#include <unistd.h>

using namespace XLib;

#ifndef WINDOWS
static auto g_pageSize = sysconf( _SC_PAGESIZE );
#endif

/**
 * auto alignedSize = ( ( size + g_pageSize ) / g_pageSize )
 *                 * g_pageSize;
 *
 * auto alignedAddress = align( address, g_pageSize );
 */

maps_t XLib::MemoryUtils::queryMaps( pid_t /*pid*/ )
{
    maps_t maps;

    /* Let's do the magic here */

    return maps;
}
