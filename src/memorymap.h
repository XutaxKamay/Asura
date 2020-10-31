#ifndef MEMORYMAP_H
#define MEMORYMAP_H

#include "memoryarea.h"
#include "memoryutils.h"

#include <memory>

namespace XLib
{
    template <class MemoryArea_T = MemoryArea>
    class MemoryMap : public std::vector<std::unique_ptr<MemoryArea_T>>
    {
    };

} // namespace XLib

#endif // MEMORYMAP_H
