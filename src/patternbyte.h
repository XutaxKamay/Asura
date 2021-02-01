#ifndef PATTERNBYTE_H
#define PATTERNBYTE_H

#include "process.h"

namespace XLib
{
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
                    std::vector<ptr_t> matches = {});

      public:
        auto values() -> std::vector<Value>&;
        auto matches() -> std::vector<ptr_t>&;
        auto isValid() -> bool;

      private:
        std::vector<Value> _values;
        std::vector<ptr_t> _matches;
    };

    using patterns_bytes_t = std::vector<PatternByte>;
};

#endif
