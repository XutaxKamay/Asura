#ifndef ASURA_MEMORYMAP_H
#define ASURA_MEMORYMAP_H

#include "memoryarea.h"
#include "memoryutils.h"

namespace Asura
{
    class ProcessMemoryMap;

    template <class C = MemoryArea>
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
        std::vector<std::shared_ptr<C>> _areas;
    };

}

#endif
