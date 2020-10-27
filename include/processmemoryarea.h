#ifndef PROCESSMEMORYAREA_H
#define PROCESSMEMORYAREA_H

#include "memoryarea.h"
#include "memoryutils.h"


namespace XLib
{
    class Process;
    class ProcessMemoryArea : public MemoryArea
    {
      public:
        ProcessMemoryArea() = default;
        ProcessMemoryArea(Process* process);

      private:
        Process* _process;
    };
};

#endif
