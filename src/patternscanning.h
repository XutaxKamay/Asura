#ifndef ASURA_PATTERNSCANNING_H
#define ASURA_PATTERNSCANNING_H

#include "process.h"

namespace Asura
{
    class PatternByte;

    class PatternScanning
    {
      public:
        static auto searchInProcess(
          PatternByte& pattern,
          const Process& process,
          const std::function<
            auto(PatternByte&, const data_t, const std::size_t, const ptr_t)
              ->bool>& searchMethod
          = searchV4) -> void;

        static auto searchInProcessWithAreaName(
          PatternByte& pattern,
          const Process& process,
          const std::string& areaName,
          const std::function<
            auto(PatternByte&, const data_t, const std::size_t, const ptr_t)
              ->bool>& searchMethod
          = searchV4) -> void;

        /**
         * This works by making the preprocessed pattern into simd
         * values, with its mask. The mask is basically used for
         * unknown bytes, so we don't need to check byte by byte.
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
                             const data_t data,
                             const std::size_t size,
                             const ptr_t baseAddress) -> bool;

        /**
         * This one is a byte by byte check, skipping the unknown
         * bytes when it can. This is traditional.
         */
        static auto searchV2(PatternByte& pattern,
                             const data_t data,
                             const std::size_t size,
                             const ptr_t baseAddress) -> bool;

        /**
         * SIMD Boyer-Moore's Algorithm.
         * Consums less memory than V4 but it is slower.
         * WARNING: please increase memory a bit more.
         * due to SIMD, if somehow the pattern has arrived
         * at the start of data, when loading into MD registers,
         * it could cause a crash if not aligned properly.
         * Though, there's rarely a problem because we read,
         * the entire area in an aligned way first.
         * (so there's pretty small chances that it crashes)
         * But to be extra safe,
         *
         * TODO:
         * 1) Align even more memory before processing (like center around
         * a page size) 2) Or check when the data left to read is smaller
         * than SIMD::value_t.
         *
         * NOTE:
         * 1) Chosen for performance, check processmemoryarea.h
         */
        static auto searchV3(PatternByte& pattern,
                             const data_t data,
                             const std::size_t size,
                             const ptr_t baseAddress) -> bool;

        /**
         * SIMD Boyer-Moore-Horspool's Algorithm.
         * Consums a lot more memory but it is faster.
         * WARNING: have same problem as V3.
         */
        static auto searchV4(PatternByte& pattern,
                             const data_t data,
                             const std::size_t size,
                             const ptr_t baseAddress) -> bool;

        /**
         * SIMD Boyer-Moore-Horspool's Algorithm, aligned.
         * This increases the number of steps because we need
         * to align the memory but also pad our pattern with unknown
         * bytes, due to the shift between the aligned data cursor and
         * current data cursor, so this increases also the pattern size
         * unfortunately.
         * Though, as an advantage, we don't have the same problem
         * as V3 and V4 (no overlap)
         */
        static auto searchAlignedV1(PatternByte& pattern,
                                    const data_t alignedData,
                                    const std::size_t size,
                                    const ptr_t baseAddress) -> bool;
    };
}

#endif
