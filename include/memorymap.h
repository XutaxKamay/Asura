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
      public:
        template <typename T = uintptr_t>
        auto search(T address)
        {
            for (auto&& area : *this)
            {
                auto start_ptr = (*area).begin();
                auto end_ptr   = (*area).end();

                if (view_as<uintptr_t>(address) >= start_ptr
                    && view_as<uintptr_t>(address) < end_ptr)
                {
                    return area;
                }
            }
        }
    };

} // namespace XLib

#endif // MEMORYMAP_H
