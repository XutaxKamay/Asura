#include "pch.h"

#include "memoryutils.h"

using namespace XKLib;

size_t MemoryUtils::_page_size;
std::once_flag MemoryUtils::_get_page_size_once_flag;

#ifdef WINDOWS
static auto _GetPageSize()
{
    SYSTEM_INFO sys_info;

    GetSystemInfo(&sys_info);

    return sys_info.dwPageSize;
}
#else
static auto _GetPageSize()
{
    return view_as<size_t>(sysconf(_SC_PAGESIZE));
}
#endif

auto MemoryUtils::GetPageSize() -> size_t
{
    std::call_once(_get_page_size_once_flag,
                   []()
                   {
                       _page_size = _GetPageSize();
                   });

    return _page_size;
}
