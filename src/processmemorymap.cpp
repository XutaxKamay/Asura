#include "processmemorymap.h"

#ifndef WINDOWS
    #include <fstream>

    #include <sys/file.h>
    #include <sys/ioctl.h>
    #include <sys/mman.h>
    #include <sys/types.h>
    #include <unistd.h>
#else
    #include <windows.h>
#endif

using namespace XLib;

ProcessMemoryMap::ProcessMemoryMap(Process* process) : _process(process)
{
    fetch();
}

auto ProcessMemoryMap::fetch() -> void
{
#ifndef WINDOWS
    std::ifstream file_memory_map(
      "/proc/" + std::to_string(_process->pid()) + "/maps");
    std::string line;

    if (!file_memory_map.is_open())
    {
        throw MemoryException("Couldn't open /proc/"
                              + std::to_string(_process->pid())
                              + "/maps");
    }

    while (std::getline(file_memory_map, line))
    {
        ProcessMemoryArea area(_process);
        uintptr_t start, end;
        byte_t prot[3];

        /* 0x0 0x1000 rwx */
        std::sscanf(line.c_str(),
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

        area.setDefaultProtectionFlags(view_as<memory_protection_flags_t>(
          (is_on(prot[0]) ? memory_protection_flags_t::READ : 0)
          | (is_on(prot[1]) ? memory_protection_flags_t::WRITE : 0)
          | (is_on(prot[2]) ? memory_protection_flags_t::EXECUTE : 0)));

        area.setAddress(view_as<ptr_t>(start));
        area.setSize(end - start);
        push_back(area);
    }

#else
    auto process_handle = OpenProcess(PROCESS_QUERY_INFORMATION,
                                      false,
        _process->pid()));

    if (process_handle == nullptr)
    {
        throw MemoryException("Couldn't open process from pid: "
            + std::to_string(_process->pid())));
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

        ProcessMemoryArea area(_process);
        area.setAddress(bs);
        area.setSize(info.RegionSize);
        area.setDefaultProtectionFlags(
          ProcessMemoryArea::Protection::toOwn(info.Protect));

        push_back(area);
    }

    CloseHandle(process_handle);
#endif
}
