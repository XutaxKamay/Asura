#include "memoryscanner.h"

using namespace XLib;

/* Singleton of MemoryScanner */
inline auto MemoryScanner::Get()
{
    static MemoryScanner memoryScanner;
    return &memoryScanner;
}

MemoryScanner* g_pMemoryScanner = MemoryScanner::Get();
