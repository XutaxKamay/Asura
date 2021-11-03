#ifndef PATTERNBYTE_H
#define PATTERNBYTE_H

#include "process.h"

#include <immintrin.h>

namespace XKLib
{
    class Process;

    /**
     * @brief The PatternByte class
     * Invalid forms:
     * UNKNOWN, byte, byte, byte, ...
     * byte, byte, byte, ..., UNKNOWN
     * Pattern should not start/finish by UNKNOWN
     */
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

#ifndef __AVX512F__
        using fastval_t = uint64_t;
#else
        using fastval_t = __m512i;
#endif

        struct FastValue
        {
            bool unknown;
            fastval_t val;
            size_t var_size;
            fastval_t mask;
        };

        PatternByte(std::vector<Value> values,
                    std::string _area_name     = "",
                    std::vector<ptr_t> matches = {});

      public:
        auto values() -> std::vector<Value>&;
        auto fvalues() -> std::vector<FastValue>&;
        auto matches() -> std::vector<ptr_t>&;
        auto isValid() -> bool;
        auto scan(Process& process) -> void;
        auto areaName() -> std::string;

      private:
        std::vector<Value> _values;
        std::vector<FastValue> _fast_values;
        std::vector<ptr_t> _matches;
        std::string _area_name;
    };

    using patterns_bytes_t = std::vector<PatternByte>;
};

#endif
