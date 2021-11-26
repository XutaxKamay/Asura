#ifndef MEMORYMAP_H
#define MEMORYMAP_H

#include "memoryarea.h"
#include "memoryutils.h"

namespace XKLib
{
    class ProcessMemoryMap;

    template <class MemoryArea_T = MemoryArea>
    class MemoryMap
    {
        friend ProcessMemoryMap;

      public:
        auto areas() -> std::vector<std::shared_ptr<MemoryArea_T>>&
        {
            return _areas;
        }

      private:
        std::vector<std::shared_ptr<MemoryArea_T>> _areas {};
    };

} // namespace XKLib

#endif // MEMORYMAP_H
