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

      private:
        /**
         * @brief _pid
         * Process ID
         */
        uint32_t _pid {};

    } extern* g_pMemoryScanner;
}

#endif // MEMORYSCANNER_H
