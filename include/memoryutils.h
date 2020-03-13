#ifndef MEMORYUTILS_H
#define MEMORYUTILS_H

#include "memorymap.h"
#include <unordered_map>

namespace XLib
{
    /**
     * @brief MemoryUtils
     * Memory utils, mainly used for making compabilities between Linux
     * and Windows API.
     */
    class MemoryUtils
    {
      public:
        static auto queryMaps( pid_t pid );
        static auto searchMap( pid_t pid, ptr_t address );
        template < typename T = ptr_t >
        static auto searchModule( T name );
    };
}

#endif // MEMORYUTILS_H
