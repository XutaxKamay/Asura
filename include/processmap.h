#ifndef PROCESSMAP_H
#define PROCESSMAP_H

#include "memorymap.h"
#include "memoryutils.h"

namespace XLib
{
    class ProcessMap
    {
      public:
        ProcessMap() = default;
        ProcessMap(pid_t pid);

      public:
        auto fetch() -> void;

        auto& operator[](int index);
        auto& operator[](const std::string& areaName);

      private:
        memory_map_t _memory_map {};
        pid_t _pid {};
    };
};

#endif
