#ifndef PATTERNBYTE_H
#define PATTERNBYTE_H

#include "simd.h"

namespace XKLib
{
    class Process;

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
            SIMD::value_t mask;
            SIMD::value_t value;
            std::size_t part_size;

            enum skip_type_t
            {
                ALL_UNKNOWN,
                ALL_KNOWN,
                MIXED
            } skip_type;
        };

        struct organized_values_t
        {
            std::vector<byte_t> bytes;
            std::size_t skip_bytes;
        };

      private:
        std::vector<Value> _bytes;
        std::vector<ptr_t> _matches;
        std::string _area_name;
        std::vector<organized_values_t> _vec_organized_values;
        std::vector<simd_mv_t> _simd_mvs;
        std::array<std::vector<std::size_t>,
                   std::numeric_limits<byte_t>::max() + 1>
          _horspool_skip_table;
        /* Shift could be max a size of simd integer value */
        std::array<std::vector<simd_mv_t>, sizeof(SIMD::value_t)>
          _shifted_simd_mvs;

      public:
        PatternByte(const std::vector<Value> values,
                    const std::string _area_name     = "",
                    const std::vector<ptr_t> matches = {});

      public:
        auto bytes() const -> const std::vector<Value>&;
        auto isValid() const -> bool;
        auto areaName() const -> const std::string&;
        auto vecOrganizedValues() const
          -> const std::vector<organized_values_t>&;
        auto simdMVs() const -> const std::vector<simd_mv_t>&;
        auto horspoolSkipTable() -> const decltype(_horspool_skip_table)&;
        auto shiftedSIMDMvs() -> const decltype(_shifted_simd_mvs)&;
        auto matches() -> std::vector<ptr_t>&;
        auto scan(const Process& process) -> void;
    };

    using patterns_bytes_t = std::vector<PatternByte>;
}

#endif
