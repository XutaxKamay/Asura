#ifndef PATTERNSCANNING_H
#define PATTERNSCANNING_H

#include "process.h"

namespace XKLib
{
    class PatternByte;

    class PatternScanning
    {
      public:
        static auto searchInProcess(
          PatternByte& pattern,
          Process& process,
          const std::function<auto(PatternByte&, const bytes_t&, ptr_t)->void>& func = searchv3)
          -> void;

        static auto searchInProcessWithAreaName(
          PatternByte& pattern,
          Process& process,
          const std::string& areaName,
          const std::function<auto(PatternByte&, const bytes_t&, ptr_t)->void>& func = searchv3)
          -> void;

        static auto search(PatternByte& pattern,
                           const bytes_t& bytes,
                           ptr_t baseAddress) -> bool;

        static auto searchv2(PatternByte& pattern,
                             const bytes_t& bytes,
                             ptr_t baseAddress) -> bool;

        static auto searchv3(PatternByte& pattern,
                             const bytes_t& bytes,
                             ptr_t baseAddress) -> bool;
    };
};

#endif
