#include "memoryutils.h"
#ifdef WINDOWS
    #include <windows.h>
#else
    #include <sys/file.h>
    #include <sys/ioctl.h>
    #include <sys/mman.h>

    #include "communicate_structs.h"
#endif
#include <unistd.h>

using namespace XLib;

#ifndef WINDOWS
static auto g_pageSize       = sysconf(_SC_PAGESIZE);
constexpr auto g_maxCountVMA = 0x400;
#endif

/**
 * auto alignedSize = ( ( size + g_pageSize ) / g_pageSize )
 *                 * g_pageSize;
 *
 * auto alignedAddress = align( address, g_pageSize );
 */

maps_t XLib::MemoryUtils::queryMaps(pid_t pid)
{
    maps_t maps;

#ifndef WINDOWS
    auto fd = open("/dev/winexts_dev", 0);

    if (fd < 0)
    {
        close(fd);
        return maps;
    }

    communicate_list_vmas_t listedVMA;

    listedVMA.pid_target    = pid;
    listedVMA.vma_max_count = g_maxCountVMA;
    listedVMA.vmas          = new communicate_vma_t[g_maxCountVMA];

    auto error = ioctl(fd, COMMUNICATE_CMD_LIST_VMAS, &listedVMA);

    if (error == COMMUNICATE_ERROR_NONE)
    {
        for (auto vmaCount = 0; vmaCount < listedVMA.vma_count;
             vmaCount++)
        {
            auto vma = &listedVMA.vmas[vmaCount];

            map_t map;

            map.setAddress(view_as<ptr_t>(vma->vm_start));
            map.setSize(view_as<size_t>(vma->vm_end - vma->vm_start));

            auto &prot = map.protection();

            prot = 0;

            if (vma->vm_flags & PROT_EXEC)
            {
                prot |= XLib::map_t::protection_t::EXECUTE;
            }

            if (vma->vm_flags & PROT_READ)
            {
                prot |= XLib::map_t::protection_t::READ;
            }

            if (vma->vm_flags & PROT_WRITE)
            {
                prot |= XLib::map_t::protection_t::WRITE;
            }

            maps.push_back(map);
        }
    }

    delete[] listedVMA.vmas;

    close(fd);
#else
    throw("Not implemented");
#endif

    return maps;
}
