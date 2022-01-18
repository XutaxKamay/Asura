#include "simd.h"

#if defined(__AVX512F__)
__m512i tmp_avx512_zeros = []()
{
    return _mm_set_pi8_simd_value(0);
}();
#endif
