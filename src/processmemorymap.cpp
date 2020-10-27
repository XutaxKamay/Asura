#include "processmemorymap.h"
#include "process.h"

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
    refresh();
}

auto ProcessMemoryMap::refresh() -> void
{
    clear();

#ifndef WINDOWS
    std::ifstream file_memory_map(
      "/proc/" + std::to_string(_process->pid()) + "/maps");
    std::string line;

    if (!file_memory_map.is_open())
    {
        throw MemoryException(
          std::string(CURRENT_CONTEXT) + "Couldn't open /proc/"
          + std::to_string(_process->pid()) + "/maps");
    }

    while (std::getline(file_memory_map, line))
    {
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

        auto area = std::make_unique<ProcessMemoryArea>(_process);
        area->initProtectionFlags((view_as<memory_protection_flags_t>(
          (is_on(prot[0]) ? memory_protection_flags_t::READ : 0)
          | (is_on(prot[1]) ? memory_protection_flags_t::WRITE : 0)
          | (is_on(prot[2]) ? memory_protection_flags_t::EXECUTE : 0))));

        area->setAddress(view_as<ptr_t>(start));
        area->setSize(end - start);

        push_back(std::move(area));
    }
#else
    auto process_handle = OpenProcess(PROCESS_QUERY_INFORMATION,
                                      false,
                                      _process->pid());

    if (process_handle == nullptr)
    {
        throw MemoryException(std::string(CURRENT_CONTEXT)
                              + "Couldn't open process from pid: "
                              + std::to_string(_process->pid()));
    }

    MEMORY_BASIC_INFORMATION info;
    data_t bs;

    for (bs = nullptr;
         VirtualQueryEx(process_handle, bs, &info, sizeof(info))
         == sizeof(info);
         bs += info.RegionSize)
    {
        auto area = std::make_unique<ProcessMemoryArea>(_process);
        area->setAddress(bs);
        area->setSize(info.RegionSize);
        area->initProtectionFlags(
          ProcessMemoryArea::Protection::toOwn(info.Protect));

        push_back(std::move(area));
    }

    CloseHandle(process_handle);
#endif
}

template <typename T>
auto ProcessMemoryMap::allocArea(T address,
                                 size_t size,
                                 memory_protection_flags_t flags) -> void
{
    MemoryUtils::AllocArea(_process->pid(), address, size, flags);
}

template <typename T>
auto ProcessMemoryMap::freeArea(T address, size_t size) -> void
{
    MemoryUtils::FreeArea(_process->pid(), address, size);
}

template <typename T>
auto ProcessMemoryMap::protectMemoryArea(T address,
                                         size_t size,
                                         memory_protection_flags_t flags)
  -> void
{
    MemoryUtils::ProtectMemoryArea(_process->pid(), address, size, flags);
}

template auto ProcessMemoryMap::protectMemoryArea<uintptr_t>(
  uintptr_t,
  size_t,
  memory_protection_flags_t) -> void;

template auto ProcessMemoryMap::protectMemoryArea<ptr_t>(
  ptr_t,
  size_t,
  memory_protection_flags_t) -> void;
