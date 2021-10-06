#ifndef PATTERNBYTE_H
#define PATTERNBYTE_H

#include "process.h"

namespace XLib
{
    class Process;

    class PatternByte
    {
      public:
        class Value
        {
          public:
            Value(int value);

            enum type_t
            {
                INVALID = -1,
                UNKNOWN = -2
            };

            int value = INVALID;
        };

        PatternByte(std::vector<Value> values,
                    std::string _area_name     = "",
                    std::vector<ptr_t> matches = {});

      public:
        auto values() -> std::vector<Value>&;
        auto matches() -> std::vector<ptr_t>&;
        auto isValid() -> bool;
        auto scan(Process process) -> void;
        auto areaName() -> std::string;

      private:
        std::vector<Value> _values;
        std::vector<ptr_t> _matches;
        std::string _area_name;
    };

    using patterns_bytes_t = std::vector<PatternByte>;
};

#endif
