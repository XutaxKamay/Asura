#ifndef PATTERNBYTE_H
#define PATTERNBYTE_H

#include "process.h"

namespace XKLib
{
    class Process;

#ifdef __AVX512F__
    using simd_value_t = __m512i;
#elif defined(__AVX2__)
    using simd_value_t = __m256i;
#elif defined(__SSE__)
    using simd_value_t = __m64;
#else
    using simd_value_t = std::uint64_t;
#endif

    /**
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
            enum
            {
                INVALID = -1,
                UNKNOWN = -2
            };

            Value(int value);

            int value = INVALID;
            std::size_t index {};
        };

        struct simd_mv_t
        {
            simd_value_t mask;
            simd_value_t value;
            std::size_t part_size;
            bool can_skip;
        };

        struct organized_values_t
        {
            std::vector<byte_t> bytes;
            std::size_t skip_bytes;
        };

        PatternByte(const std::vector<Value> values,
                    const std::string _area_name     = "",
                    const std::vector<ptr_t> matches = {});

      public:
        auto bytes() const -> const std::vector<Value>&;
        auto isValid() const -> bool;
        auto areaName() const -> const std::string&;
        auto vecOrganizedValues() const
          -> const std::vector<organized_values_t>&;
        auto fastAlignedMVs() const -> const std::vector<simd_mv_t>&;

      public:
        auto matches() -> std::vector<ptr_t>&;
        auto scan(const Process& process) -> void;

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
}

#endif
