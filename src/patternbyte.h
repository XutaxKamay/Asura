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

        PatternByte(std::vector<std::shared_ptr<Value>> values,
                    std::string _area_name     = "",
                    std::vector<ptr_t> matches = {});

        ~PatternByte();

      public:
        auto values() -> std::vector<std::shared_ptr<Value>>&;
        auto simd_unknown_values() -> std::vector<simd_unknown_value_t>&;
        auto matches() -> std::vector<ptr_t>&;
        auto isValid() -> bool;
        auto scan(Process& process) -> void;
        auto areaName() -> std::string;
        auto vec_known_values()
          -> std::vector<std::shared_ptr<std::vector<byte_t>>>&;
        auto vec_skipper_uk_values() -> std::vector<size_t>&;
        auto fast_aligned_values() -> simd_value_t*;
        auto fast_aligned_masks() -> simd_value_t*;
        auto fast_aligned_values_count() -> size_t;

      private:
        std::vector<std::shared_ptr<Value>> _values;
        std::vector<simd_unknown_value_t> _unknown_values;
        std::vector<ptr_t> _matches;
        std::string _area_name;
        std::vector<std::shared_ptr<std::vector<byte_t>>> _vec_known_values;
        std::vector<size_t> _vec_skipper_uk_values;
        simd_value_t* _fast_aligned_values;
        simd_value_t* _fast_aligned_masks;
        size_t _fast_aligned_values_count;
    };

    using patterns_bytes_t = std::vector<PatternByte>;
};

#endif
