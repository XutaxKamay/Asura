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

            enum
            {
                INVALID = -1,
                UNKNOWN = -2
            };

            int value = INVALID;
            size_t index {};
        };

#ifdef __AVX512F__
        using fastval_t = __m512i;
#elif defined(__AVX2__)
        using fastval_t = __m256i;
#else
        using fastval_t = uint64_t;
#endif
        PatternByte(std::vector<std::shared_ptr<Value>> values,
                    std::string _area_name     = "",
                    std::vector<ptr_t> matches = {});

      public:
        auto values() -> std::vector<std::shared_ptr<Value>>&;
        auto unknown_values() -> std::vector<size_t>&;
        auto raw_values() -> bytes_t&;
        auto matches() -> std::vector<ptr_t>&;
        auto isValid() -> bool;
        auto scan(Process& process) -> void;
        auto areaName() -> std::string;

      private:
        std::vector<std::shared_ptr<Value>> _values;
        std::vector<size_t> _unknown_values;
        bytes_t _raw_values;
        std::vector<ptr_t> _matches;
        std::string _area_name;
    };

    using patterns_bytes_t = std::vector<PatternByte>;
};

#endif
