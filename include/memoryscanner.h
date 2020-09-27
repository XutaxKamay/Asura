#ifndef MEMORYSCANNER_H
#define MEMORYSCANNER_H

#include "memoryutils.h"

namespace XLib
{
    /**
     * @brief MemoryScanner
     * Permits to scan the memory and retrieve offsets, memory pointers
     * etc.
     */
    class MemoryScanner
    {
        /* Static methods */
      public:
        /**
         * @brief Get
         *
         * @return XLib::MemoryScanner*
         */
        static auto Get();

    } extern* g_pMemoryScanner;
} // namespace XLib

#endif // MEMORYSCANNER_H
