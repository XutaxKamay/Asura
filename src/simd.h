#ifndef SIMD_H
#define SIMD_H

/* no movemask for avx512, Intel u fkr. */
#if defined(__AVX512F__)
__m512i tmp_avx512_zeros;
#endif

#if defined(__AVX512F__)
    #define _mm_movemask_simd_value(mm1)                                 \
        _mm512_cmpeq_epi8_mask(mm1, tmp_avx512_zeros)
#elif defined(__AVX2__)
    #define _mm_movemask_simd_value(mm1) _mm256_movemask_epi8(mm1)
#else
    #define _mm_movemask_simd_value(mm1) _mm_movemask_pi8(mm1)
#endif

#if defined(__AVX512F__)
    #define _mm_cmp_pi8_simd_value(mm1, mm2)                             \
        _mm512_cmpeq_epi8_mask(mm1, mm2)
#elif defined(__AVX2__)
    #define _mm_cmp_pi8_simd_value(mm1, mm2)                             \
        _mm256_movemask_epi8(_mm256_cmpeq_epi8(mm1, mm2))
#else
    #define _mm_cmp_pi8_simd_value(mm1, mm2)                             \
        _mm_movemask_pi8(_mm_cmpeq_pi8(mm1, mm2))
#endif

#if defined(__AVX512F__)
    #define _mm_and_simd_value(mm1, mm2) _mm512_and_si512(mm1, mm2)
#elif defined(__AVX2__)
    #define _mm_and_simd_value(mm1, mm2) _mm256_and_si256(mm1, mm2)
#else
    #define _mm_and_simd_value(mm1, mm2) _mm_and_si64(mm1, mm2)
#endif

#if defined(__AVX512F__)
    #define _mm_load_simd_value(mm1)                                     \
        _mm512_load_si512(view_as<PatternByte::simd_value_t*>(mm1))
#elif defined(__AVX2__)
    #define _mm_load_simd_value(mm1)                                     \
        _mm256_load_si256(view_as<PatternByte::simd_value_t*>(mm1))
#else
    #define _mm_load_simd_value(mm1)                                     \
        _mm_cvtsi64_m64(*view_as<uint64_t*>(mm1))
#endif

#if defined(__AVX512F__)
    #define _mm_loadu_simd_value(mm1)                                    \
        _mm512_loadu_si512(view_as<PatternByte::simd_value_t*>(mm1))
#elif defined(__AVX2__)
    #define _mm_loadu_simd_value(mm1)                                    \
        _mm256_loadu_si256(view_as<PatternByte::simd_value_t*>(mm1))
#else
    #define _mm_loadu_simd_value(mm1)                                    \
        _mm_cvtsi64_m64(*view_as<uint64_t*>(mm1))
#endif

#if defined(__AVX512F__)
    #define _mm_set_pi8_simd_value(xx) _mm512_set1_epi8(xx)
#elif defined(__AVX2__)
    #define _mm_set_pi8_simd_value(xx) _mm256_set1_epi8(xx)
#else
    #define _mm_set_pi8_simd_value(xx) _mm_set1_pi8(xx)
#endif

#endif
