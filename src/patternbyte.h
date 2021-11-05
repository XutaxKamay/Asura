#ifndef PATTERNBYTE_H
#define PATTERNBYTE_H

#define PATTERN_UNALIGNED_SIMD_V1_V2

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
        using simd_value_t = __m512i;
#elif defined(__AVX2__)
        using simd_value_t = __m256i;
#else
        using simd_value_t = uint64_t;
#endif

        struct simd_unknown_value_t
        {
            size_t simd_index;
            size_t simd_byte_index;
            size_t data_byte_index;
            size_t size_to_copy;
        };

        struct fast_value_t
        {
            int unknown;
            size_t var_size;
            simd_value_t value;
            simd_value_t mask;
        };

        PatternByte(std::vector<std::shared_ptr<Value>> values,
                    std::string _area_name     = "",
                    std::vector<ptr_t> matches = {});
        ~PatternByte();

      public:
        auto values() -> std::vector<std::shared_ptr<Value>>&;
        auto fvalues() -> std::vector<fast_value_t>&;
        auto simd_unknown_values() -> std::vector<simd_unknown_value_t>&;
        auto simd_values() -> simd_value_t*;
        auto simd_values_size() -> size_t&;
        auto matches() -> std::vector<ptr_t>&;
        auto isValid() -> bool;
        auto scan(Process& process) -> void;
        auto areaName() -> std::string;
        auto fast_masks() -> simd_value_t*;
        auto vec_known_values() -> std::vector<std::vector<byte_t>>&;
        auto vec_skipper_uk_values() -> std::vector<size_t>&;

      private:
        simd_value_t* _fast_masks;
        std::vector<std::shared_ptr<Value>> _values;
        std::vector<simd_unknown_value_t> _unknown_values;
        simd_value_t* _simd_values {};
        size_t _simd_values_size {};
        std::vector<fast_value_t> _fast_values;
        std::vector<ptr_t> _matches;
        std::string _area_name;
        std::vector<std::vector<byte_t>> _vec_known_values;
        std::vector<size_t> _vec_skipper_uk_values;
    };

    using patterns_bytes_t = std::vector<PatternByte>;
};

#endif
