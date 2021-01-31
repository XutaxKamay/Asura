#ifndef PATTERNSCANNING_H
#define PATTERNSCANNING_H

#include "pattern.h"
#include "process.h"

namespace XLib
{
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
        static auto searchInPID(Pattern& pattern, pid_t pid) -> void;

        static auto search(Pattern& pattern,
                           bytes_t bytes,
                           ptr_t baseAddress) -> bool;
    };
};

#endif
