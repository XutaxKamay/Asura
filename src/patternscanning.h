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
          const std::function<auto(PatternByte&, data_t, size_t, ptr_t)->bool>& func = searchAlignedV2)
          -> void;

        static auto searchInProcessWithAreaName(
          PatternByte& pattern,
          Process& process,
          const std::string& areaName,
          const std::function<auto(PatternByte&, data_t, size_t, ptr_t)->bool>& func = searchAlignedV2)
          -> void;

        static auto searchV1(PatternByte& pattern,
                             data_t data,
                             size_t size,
                             ptr_t baseAddress) -> bool;

        static auto searchV2(PatternByte& pattern,
                             data_t data,
                             size_t size,
                             ptr_t baseAddress) -> bool;

        static auto searchV3(PatternByte& pattern,
                             data_t data,
                             size_t size,
                             ptr_t baseAddress) -> bool;

        static auto searchV4(PatternByte& pattern,
                             data_t data,
                             size_t size,
                             ptr_t baseAddress) -> bool;

        static auto searchAlignedV1(PatternByte& pattern,
                                  data_t aligned_data,
                                  size_t size,
                                  ptr_t baseAddress) -> bool;

        static auto searchAlignedV2(PatternByte& pattern,
                                    data_t aligned_data,
                                    size_t size,
                                    ptr_t baseAddress) -> bool;
    };
};

#endif
