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

        struct unknown_value_t
        {
            size_t simd_index;
            size_t byte_index;
            size_t index;
        };

#ifdef __AVX512F__
        using simd_value_t = __m512i;
#elif defined(__AVX2__)
        using simd_value_t = __m256i;
#else
        using simd_value_t = uint64_t;
#endif

        PatternByte(std::vector<std::shared_ptr<Value>> values,
                    std::string _area_name     = "",
                    std::vector<ptr_t> matches = {});

      public:
        auto values() -> std::vector<std::shared_ptr<Value>>&;
        auto unknown_values() -> std::vector<unknown_value_t>&;
        auto simd_values() -> std::vector<simd_value_t>&;
        auto aligned_simd_values() -> std::vector<simd_value_t>;
        auto matches() -> std::vector<ptr_t>&;
        auto isValid() -> bool;
        auto scan(Process& process) -> void;
        auto areaName() -> std::string;

      private:
        std::vector<std::shared_ptr<Value>> _values;
        std::vector<unknown_value_t> _unknown_values;
        std::vector<simd_value_t> _simd_values {};
        std::vector<ptr_t> _matches;
        std::string _area_name;
    };

    using patterns_bytes_t = std::vector<PatternByte>;
};

#endif
