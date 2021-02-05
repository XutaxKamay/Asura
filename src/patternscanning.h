#ifndef PATTERNSCANNING_H
#define PATTERNSCANNING_H

#include "process.h"

namespace XLib
{
    class PatternByte;

    class PatternScanningException : std::exception
    {
      public:
        PatternScanningException(const std::string& msg);

        auto msg() -> const std::string&;

      private:
        std::string _msg {};
    };

    class PatternScanning
    {
      public:
        static auto searchInProcess(PatternByte& pattern, Process process)
          -> void;

        static auto search(PatternByte& pattern,
                           bytes_t bytes,
                           ptr_t baseAddress) -> bool;
    };
};

#endif
