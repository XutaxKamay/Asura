#ifndef PATTERNSCANNING_H
#define PATTERNSCANNING_H

#include "process.h"

namespace XLib
{
    class PatternByte;

    class PatternScanning
    {
      public:
        static auto searchInProcess(PatternByte& pattern,
                                    Process& process) -> void;

        static auto searchInProcessWithAreaName(
          PatternByte& pattern,
          Process& process,
          const std::string& areaName) -> void;

        static auto search(PatternByte& pattern,
                           bytes_t bytes,
                           ptr_t baseAddress) -> bool;
    };
};

#endif
