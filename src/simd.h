#ifndef SIMD_H
#define SIMD_H

#include "types.h"

#define XKLIB_SIMD_THROW_ERROR #error "SSE/AVX2/AVX512F only supported"

#if defined(__AVX512F__) or defined(__AVX2__) or defined(__SSE__)
    #define XKLIB_HAS_SIMD
#endif

namespace XKLib
{
    inline auto mm_set_epi8_simd(const auto xx)
    {
#if defined(__AVX512F__)
        return _mm512_set1_epi8(xx);
#elif defined(__AVX2__)
        return _mm256_set1_epi8(xx);
#elif defined(__SSE__)
        return _mm_set1_pi8(xx);
#else
        return 0;
#endif
    }

    inline auto mm_movemask_epi8(const auto mm1)
    {
#if defined(__AVX512F__)
        const auto mm2 = mm_set_pi8_simd(0);
        return _mm512_cmpeq_epi8_mask(mm1, mm2);
#elif defined(__AVX2__)
        return _mm256_movemask_epi8(mm1);
#elif defined(__SSE__)
        return _mm_movemask_pi8(mm1);
#else
        /* Search cross-platform builtin for this */
        std::int8_t ret = 0;

        for (std::int8_t i = 0; i < sizeof(mm1); i++)
        {
            if (view_as<byte_t*>(mm1)[i] & 0x40)
            {
                ret |= 1 << i;
            }
        }

        return ret;
#endif
    }

    inline auto mm_cmp_epi8_mask_simd(const auto mm1, const auto mm2)
    {
#if defined(__AVX512F__)
        return _mm512_cmpeq_epi8_mask(mm1, mm2);
#elif defined(__AVX2__)
        return mm_movemask_epi8(_mm256_cmpeq_epi8(mm1, mm2));
#elif defined(__SSE__)
        return mm_movemask_epi8(_mm_cmpeq_pi8(mm1, mm2));
#else
        return mm1 == mm2;
#endif
    }

    inline auto mm_and_simd(const auto mm1, const auto mm2)
    {
#if defined(__AVX512F__)
        return _mm512_and_si512(mm1, mm2);
#elif defined(__AVX2__)
        return _mm256_and_si256(mm1, mm2);
#elif defined(__SSE__)
        return _mm_and_si64(mm1, mm2);
#else
        return mm1 & mm2;
#endif
    }

    inline auto mm_load_simd(const auto mm1)
    {
#if defined(__AVX512F__)
        return _mm512_load_si512(view_as<__m512i*>(mm1));
#elif defined(__AVX2__)
        return _mm256_load_si256(view_as<__m256i*>(mm1));
#elif defined(__SSE__)
        return *mm1;
#else
        return *mm1;
#endif
    }

    inline auto mm_loadu_simd(const auto mm1)
    {
#if defined(__AVX512F__)
        return _mm512_loadu_si512(view_as<__m512i*>(mm1));
#elif defined(__AVX2__)
        return _mm256_loadu_si256(view_as<__m256i*>(mm1));
#elif defined(__SSE__)
        return *mm1;
#else
        return *mm1;
#endif
    }
}

#endif
