#include "memoryutils.h"
#include <mutex>
#include <thread>

using namespace XLib;

size_t MemoryUtils::_page_size;

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
    static std::once_flag flag;
    std::call_once(flag,
                   []()
                   {
                       _page_size = _GetPageSize();
                   });

    return _page_size;
}
