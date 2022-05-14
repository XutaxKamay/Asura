#ifndef SIMD_H
#define SIMD_H

#include "types.h"
#include <emmintrin.h>
#include <immintrin.h>

#define XKLIB_SIMD_THROW_ERROR #error "SSE/AVX2/AVX512F only supported"

#if defined(__AVX512BW__) or defined(__AVX2__) or defined(__SSE__)
    #define XKLIB_HAS_SIMD
#endif

namespace XKLib
{
    class SIMD
    {
      public:
#if defined(__AVX512BW__)
        using value_t                        = __m512i;
        static constexpr std::size_t cmp_all = std::numeric_limits<
          uint64_t>::max();
#elif defined(__AVX2__)
        using value_t                        = __m256i;
        static constexpr std::size_t cmp_all = std::numeric_limits<
          uint32_t>::max();
#elif defined(__SSE2__)
        using value_t                        = __m128i;
        static constexpr std::size_t cmp_all = std::numeric_limits<
          uint16_t>::max();
#elif defined(__SSE__)
        using value_t                        = __m64;
        static constexpr std::size_t cmp_all = std::numeric_limits<
          uint8_t>::max();
#else
        using value_t                        = std::uint64_t;
        static constexpr std::size_t cmp_all = std::numeric_limits<
          uint8_t>::max();
#endif

        static inline auto Set8bits(const auto xx)
        {
#if defined(__AVX512BW__)
            return _mm512_set1_epi8(xx);
#elif defined(__AVX2__)
            return _mm256_set1_epi8(xx);
#elif defined(__SSE2__)
            return _mm_set1_epi8(xx);
#elif defined(__SSE__)
            return _set1_pi8(xx);
#else
            value_t value;

            for (std::size_t i = 0; i < sizeof(value_t); i++)
            {
                view_as<byte_t*>(&value)[i] = xx;
            }

            return value;
#endif
        }

        static inline auto MoveMask8bits(const auto mm1)
        {
#if defined(__AVX512BW__)
            const auto mm2 = _mm512_set1_epi8(0);
            return _mm512_cmpeq_epi8_mask(mm1, mm2);
#elif defined(__AVX2__)
            return _mm256_movemask_epi8(mm1);
#elif defined(__SSE2__)
            return _mm_movemask_epi8(mm1);
#elif defined(__SSE__)
            return _movemask_pi8(mm1);
#else
            /* Search cross-platform builtin for this */
            typename std::remove_cv<decltype(mm1)>::type ret = 0;

            for (std::size_t i = 0; i < sizeof(decltype(mm1)); i++)
            {
                if (view_as<byte_t*>(&mm1)[i] & 0x80)
                {
                    ret |= (1 << view_as<decltype(ret)>(i));
                }
            }

            return ret;
#endif
        }

        static inline auto CMPMask8bits(const auto mm1, const auto mm2)
        {
#if defined(__AVX512BW__)
            return _mm512_cmpeq_epi8_mask(mm1, mm2);
#elif defined(__AVX2__)
            return MoveMask8bits(_mm256_cmpeq_epi8(mm1, mm2));
#elif defined(__SSE2__)
            /* RIP _mm_cmpeq_epi8_mask only supported with AVX512 */
            return MoveMask8bits(_mm_cmpeq_epi8(mm1, mm2));
#elif defined(__SSE__)
            return MoveMask8bits(_cmpeq_pi8(mm1, mm2));
#else
            static_assert(sizeof(mm1) == sizeof(mm2), "not same size");

            typename std::remove_cv<decltype(mm1)>::type result;

            for (std::size_t i = 0; i < sizeof(mm1); i++)
            {
                if (view_as<byte_t*>(&mm1)[i]
                    == view_as<byte_t*>(&mm2)[i])
                {
                    view_as<byte_t*>(&result)[i] = 0xFF;
                }
                else
                {
                    view_as<byte_t*>(&result)[i] = 0x00;
                }
            }

            return MoveMask8bits(result);
#endif
        }

        static inline auto And(const auto mm1, const auto mm2)
        {
#if defined(__AVX512BW__)
            return _mm512_and_si512(mm1, mm2);
#elif defined(__AVX2__)
            return _mm256_and_si256(mm1, mm2);
#elif defined(__SSE2__)
            return _mm_and_si128(mm1, mm2);
#elif defined(__SSE__)
            return _and_si64(mm1, mm2);
#else
            return mm1 & mm2;
#endif
        }

        static inline auto Load(const auto mm1)
        {
#if defined(__AVX512BW__)
            return _mm512_load_si512(view_as<__m512i*>(mm1));
#elif defined(__AVX2__)
            return _mm256_load_si256(view_as<__m256i*>(mm1));
#elif defined(__SSE2__)
            return _mm_load_si128(mm1);
#elif defined(__SSE__)
            return *mm1;
#else
            return *mm1;
#endif
        }

        static inline auto LoadUnaligned(const auto mm1)
        {
#if defined(__AVX512BW__)
            return _mm512_loadu_si512(view_as<__m512i*>(mm1));
#elif defined(__AVX2__)
            return _mm256_loadu_si256(view_as<__m256i*>(mm1));
#elif defined(__SSE2__)
            return _mm_loadu_si128(mm1);
#elif defined(__SSE__)
            return *mm1;
#else
            return *mm1;
#endif
        }
    };
}

#endif
