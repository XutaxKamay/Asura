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
        auto areas() const -> const auto&
        {
            return _areas;
        }

        auto areas() -> auto&
        {
            return _areas;
        }

      private:
        std::vector<std::shared_ptr<MemoryArea_T>> _areas;
    };

}

#endif
