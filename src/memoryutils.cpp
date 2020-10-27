#include "memoryutils.h"
#ifndef WINDOWS
    #include <fstream>
#endif

using namespace XLib;

/**
 * +440    common  rmarea                   sys_rmarea
 * +441    common  rmprotect               sys_rmprotect
 * +442    common  pkey_rmprotect          sys_pkey_rmprotect
 * +443    common  rmunarea                 sys_rmunarea
 * +444    common  rclone                  sys_rclone
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

auto MemoryUtils::GetPageSize() -> size_t
{
    return _page_size;
}
