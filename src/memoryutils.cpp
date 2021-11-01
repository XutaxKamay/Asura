#include "memoryutils.h"

using namespace XLib;

size_t MemoryUtils::_page_size;
std::once_flag MemoryUtils::_memory_page_once_flag;

#ifdef WINDOWS
static auto _GetPageSize()
{
    SYSTEM_INFO sys_info;

    GetSystemInfo(&sys_info);

    return sys_info.dwPageSize;
}

size_t MemoryUtils::_page_size = GetPageSize();
#else
static auto _GetPageSize()
{
    return view_as<size_t>(sysconf(_SC_PAGESIZE));
}
#endif

auto MemoryUtils::GetPageSize() -> size_t
{
    std::call_once(_memory_page_once_flag,
                   []()
                   {
                       _page_size = _GetPageSize();
                   });

    return _page_size;
}
