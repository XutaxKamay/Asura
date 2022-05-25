#ifndef XKLIB_PATTERNBYTE_H
#define XKLIB_PATTERNBYTE_H

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

        struct SIMDMaskValue
        {
            SIMD::value_t mask;
            SIMD::value_t value;
            std::size_t part_size;

            enum SkipType
            {
                ALL_UNKNOWN,
                ALL_KNOWN,
                MIXED
            } skip_type;
        };

        using simd_masks_values_t = std::vector<SIMDMaskValue>;

        using shifted_simd_masks_values_t = std::
          array<simd_masks_values_t, sizeof(SIMD::value_t)>;

        using horspool_table_t = std::array<
          std::vector<std::size_t>,
          std::numeric_limits<byte_t>::max() + 1>;

        struct OrganizedValues
        {
            std::vector<byte_t> bytes;
            std::size_t skip_bytes;
        };

      private:
        std::vector<Value> _bytes;
        std::vector<ptr_t> _matches;
        std::string _area_name;
        std::vector<OrganizedValues> _vec_organized_values;
        simd_masks_values_t _simd_masks_values;
        horspool_table_t _horspool_skip_table;
        /* Shift could be max a size of simd integer value */
        shifted_simd_masks_values_t _shifted_simd_masks_values;

      public:
        PatternByte(const std::vector<Value> values,
                    const std::string _area_name     = "",
                    const std::vector<ptr_t> matches = {});

      private:
        auto setupOrganizedValues() -> void;
        auto setupSIMDMaskValues(simd_masks_values_t& simd_masks_values,
                                 std::vector<Value>& bytes) -> void;
        auto setupSIMDMaskValues() -> void;
        auto setupHorspoolTable(horspool_table_t& horspoolTable,
                                simd_masks_values_t& simdMasksValues,
                                std::vector<Value>& bytes) -> void;

      public:
        auto bytes() const -> const std::vector<Value>&;
        auto isValid() const -> bool;
        auto areaName() const -> const std::string&;
        auto vecOrganizedValues() const
          -> const std::vector<OrganizedValues>&;
        auto simdMasksValues() const -> const simd_masks_values_t&;
        auto horspoolSkipTable() -> const horspool_table_t&;
        auto shiftedSIMDMasksValues()
          -> const shifted_simd_masks_values_t&;
        auto matches() -> std::vector<ptr_t>&;
        auto scan(const Process& process) -> void;
    };

    using patterns_bytes_t = std::vector<PatternByte>;
}

#endif
