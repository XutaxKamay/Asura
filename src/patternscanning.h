#ifndef PATTERNSCANNING_H
#define PATTERNSCANNING_H

#include "process.h"

namespace XKLib
{
    class PatternByte;

    class PatternScanning
    {
      public:
        static auto searchInProcess(
          PatternByte& pattern,
          Process& process,
          const std::function<auto(PatternByte&, data_t, size_t, ptr_t)->bool>& searchMethod = searchV1)
          -> void;

        static auto searchInProcessWithAreaName(
          PatternByte& pattern,
          Process& process,
          const std::string& areaName,
          const std::function<auto(PatternByte&, data_t, size_t, ptr_t)->bool>& searchMethod = searchV1)
          -> void;

        /**
         * @brief searchV1
         * @param pattern
         * @param data
         * @param size
         * @param baseAddress
         * @return
         * This works by making the preprocessed pattern into simd values,
         * with its mask. The mask is basically used for unknown bytes, so
         * we don't need to check byte by byte.
         * Example for 32 bits:
         *
         * data = 0xDEADC0DE
         * pattern = 0xDE??CO??
         *
         * We can't do straight data == pattern, but we can do:
         * (data & 0xFF00FF00) == (0xDE??C0?? & 0xFF00FF00)
         * => 0xDE00CO00 == 0xDE00CO00, so we get a match here.
         *
         * Of course the pattern is preprocessed, so we gain one
         * instruction aka (data & 0xFF00FF00) == 0xDE00CO00.
         */
        static auto searchV1(PatternByte& pattern,
                             data_t data,
                             size_t size,
                             ptr_t baseAddress) -> bool;

        /**
         * @brief searchV2
         * @param pattern
         * @param data
         * @param size
         * @param baseAddress
         * @return
         *
         * This one is a byte by byte check, skipping the unknown bytes
         * when it can. This is traditional.
         */
        static auto searchV2(PatternByte& pattern,
                             data_t data,
                             size_t size,
                             ptr_t baseAddress) -> bool;
        /**
         * @brief searchV3
         * @param pattern
         * @param data
         * @param size
         * @param baseAddress
         * @return
         *
         * Same as v2 but uses SIMD for checking bytes.
         */
        static auto searchV3(PatternByte& pattern,
                             data_t data,
                             size_t size,
                             ptr_t baseAddress) -> bool;

        /**
         * @brief searchAlignedV1
         * @param pattern
         * @param aligned_data
         * @param size
         * @param baseAddress
         * @return
         *
         * This one is the same as V2, but works on pattern that was
         * previously taken on aligned code. This is extremely fast.
         */
        static auto searchAlignedV1(PatternByte& pattern,
                                    data_t aligned_data,
                                    size_t size,
                                    ptr_t baseAddress) -> bool;

        /**
         * @brief searchAlignedV2
         * @param pattern
         * @param aligned_data
         * @param size
         * @param baseAddress
         * @return
         *
         * This one is the same as V1, but works on pattern that was
         * previously taken on aligned code. This is extremely fast.
         */
        static auto searchAlignedV2(PatternByte& pattern,
                                    data_t aligned_data,
                                    size_t size,
                                    ptr_t baseAddress) -> bool;

        /**
         * V3 can't be implemented as aligned because in order to load
         * simd value, the memory is at some point not aligned due to the
         * bytes we skip
         */

        /**
         * @brief searchTest
         * @param pattern
         * @param aligned_data
         * @param size
         * @param baseAddress
         * @return
         *
         * This is the one who has been copied over and over again.
         */
        static auto searchTest(PatternByte& pattern,
                               data_t data,
                               size_t size,
                               ptr_t baseAddress) -> bool;
    };
};

#endif
