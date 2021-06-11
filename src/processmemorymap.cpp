#include "processmemorymap.h"
#include "process.h"
#include <utility>

#ifndef WINDOWS
    #include <fstream>
    #include <unistd.h>

    #include <sys/file.h>
    #include <sys/ioctl.h>
    #include <sys/mman.h>
    #include <sys/types.h>
#else
    #include <windows.h>
#endif

using namespace XLib;

ProcessMemoryMap::ProcessMemoryMap(ProcessBase process)
 : _process_base(process)
{
    refresh();
}

auto ProcessMemoryMap::refresh() -> void
{
    if (_process_base.id() == Process::INVALID_PID)
    {
        return;
    }

    _areas.clear();

#ifndef WINDOWS
    std::ifstream file_memory_map(
      "/proc/" + std::to_string(_process_base.id()) + "/maps");
    std::string line;

    if (!file_memory_map.is_open())
    {
        XLIB_EXCEPTION("Couldn't open /proc/"
                             + std::to_string(_process_base.id())
                             + "/maps");
    }

    while (std::getline(file_memory_map, line))
    {
        uintptr_t start, end;
        byte_t prot[3];
        std::string name = "unknown";

        /* 0x0 0x1000 rwxp %x %i:%i %i %s */
        std::sscanf(line.c_str(),
                    "%p-%p %c%c%c",
                    (ptr_t*)&start,
                    (ptr_t*)&end,
                    &prot[0],
                    &prot[1],
                    &prot[2]);

        if (line[line.size() - 1] != '0')
        {
            size_t count_char = 1;

            while (line[line.size() - count_char] != ' ')
            {
                count_char++;
            }

            name = std::string(line.begin()
                                 + (line.size() - count_char + 1),
                               line.end());
        }

        auto is_on = [](byte_t prot)
        {
            if (prot == '-')
            {
                return false;
            }

            return true;
        };

        auto area = std::make_shared<ProcessMemoryArea>(_process_base);
        area->initProtectionFlags(
          (is_on(prot[0]) ? MemoryArea::ProtectionFlags::READ : 0)
          | (is_on(prot[1]) ? MemoryArea::ProtectionFlags::WRITE : 0)
          | (is_on(prot[2]) ? MemoryArea::ProtectionFlags::EXECUTE : 0));
        area->setAddress(view_as<ptr_t>(start));
        area->setSize(end - start);
        area->setName(name);

        _areas.push_back(std::move(area));
    }

    file_memory_map.close();

#else
    auto process_handle = OpenProcess(PROCESS_QUERY_INFORMATION,
                                      false,
                                      _process_base.id());

    if (process_handle == nullptr)
    {
        XLIB_EXCEPTION("Couldn't open process from pid: "
                             + std::to_string(_process_base.id()));
    }

    MEMORY_BASIC_INFORMATION info;
    data_t bs;
    char module_path[MAX_PATH];

    for (bs = nullptr;
         VirtualQueryEx(process_handle, bs, &info, sizeof(info))
         == sizeof(info);
         bs += info.RegionSize)
    {
        auto area = std::make_shared<ProcessMemoryArea>(_process_base);
        area->setAddress(bs);
        area->setSize(info.RegionSize);
        area->initProtectionFlags(
          ProcessMemoryArea::ProtectionFlags::ToOwn(info.Protect));

        if (GetModuleFileNameA(view_as<HMODULE>(info.AllocationBase),
                               module_path,
                               sizeof(module_path)))
        {
            area->setName(module_path);
        }
        else
        {
            area->setName("unknown");
        }

        _areas.push_back(std::move(area));
    }

    CloseHandle(process_handle);
#endif
}
