#ifndef PATTERNBYTE_H
#define PATTERNBYTE_H

#include "process.h"

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
            std::size_t index {};
        };

#ifdef __AVX512F__
        using simd_value_t = __m512i;
#elif defined(__AVX2__)
        using simd_value_t = __m256i;
#else
        using simd_value_t = uint64_t;
#endif
        struct simd_mv_t
        {
            simd_value_t mask;
            simd_value_t value;
            std::size_t part_size;
        };

        struct organized_values_t
        {
            std::vector<byte_t> bytes;
            std::size_t skip_bytes;
        };

        PatternByte(std::vector<Value> values,
                    std::string _area_name     = "",
                    std::vector<ptr_t> matches = {});

      public:
        auto bytes() -> std::vector<Value>&;
        auto matches() -> std::vector<ptr_t>&;
        auto isValid() -> bool;
        auto scan(Process& process) -> void;
        auto areaName() -> std::string;
        auto vec_organized_values() -> std::vector<organized_values_t>&;
        auto fast_aligned_mvs() -> std::vector<simd_mv_t>&;

        std::vector<std::size_t>
          skip_table[std::numeric_limits<byte_t>::max() + 1];

      private:
        std::vector<Value> _bytes;
        std::vector<ptr_t> _matches;
        std::string _area_name;
        std::vector<organized_values_t> _vec_organized_values;
        std::vector<simd_mv_t> _fast_aligned_mvs;
    };

    using patterns_bytes_t = std::vector<PatternByte>;
};

#endif
