#include <utility>

#include "patternscanning.h"
#include "process.h"

#include <cstdlib>
#include <cstring>

XKLib::PatternByte::Value::Value(int value) : value(value)
{
}

XKLib::PatternByte::PatternByte(std::vector<std::shared_ptr<Value>> values,
                                std::string areaName,
                                std::vector<ptr_t> matches)
 : _values(std::move(values)), _matches(std::move(matches)),
   _area_name(std::move(areaName))
{
    if (!isValid())
    {
        XKLIB_EXCEPTION("Invalid pattern.");
    }

    /**
     *  Let's do some preprocessing now *
     */

    /* First scanning method, using filling bytes */
    {
        _unknown_values.reserve(_values.size());

        /* Allocate simd values in aligned way for optimizations */
        _simd_values = view_as<simd_value_t*>(
          std::aligned_alloc(sizeof(simd_value_t), _values.size()));
        _fast_masks = view_as<simd_value_t*>(
          std::aligned_alloc(sizeof(simd_value_t), _values.size()));

        auto left = _values.size() % sizeof(simd_value_t);

        if (left > 0)
        {
            left = sizeof(simd_value_t) - left;
        }

        _simd_values_size = _values.size() + left;

        size_t index                       = 0;
        size_t ukval_contigous_count       = 0;
        size_t index_since_contigous_count = 0;
        size_t byte_simd_index             = 0;
        bool are_first_known_values        = true;
        simd_value_t mask {};

        for (auto&& value : _values)
        {
            value->index = index;

            if (value->value == Value::UNKNOWN)
            {
                index_since_contigous_count = index;
                ukval_contigous_count++;

                view_as<byte_t*>(&mask)[byte_simd_index] = 0x00;

                if (ukval_contigous_count >= sizeof(simd_value_t))
                {
                    _unknown_values.push_back(
                      { index_since_contigous_count / sizeof(simd_value_t),
                        index_since_contigous_count % sizeof(simd_value_t),
                        index_since_contigous_count,
                        ukval_contigous_count });
                    ukval_contigous_count = 0;
                }

                view_as<byte_t*>(_simd_values)[index] = 0;
                are_first_known_values                = false;
            }
            else
            {
                if (are_first_known_values)
                {
                    _first_known_values.push_back(
                      view_as<byte_t>(value->value));
                }

                if (ukval_contigous_count > 0)
                {
                    _unknown_values.push_back(
                      { index_since_contigous_count / sizeof(simd_value_t),
                        index_since_contigous_count % sizeof(simd_value_t),
                        index_since_contigous_count,
                        ukval_contigous_count });
                    ukval_contigous_count = 0;
                }

                view_as<byte_t*>(&mask)[byte_simd_index] = 0xFF;

                view_as<byte_t*>(_simd_values)[index] = view_as<byte_t>(
                  value->value);
            }

            index++;
            byte_simd_index++;

            if (byte_simd_index >= sizeof(simd_value_t))
            {
                _fast_masks[index / sizeof(simd_value_t)] = mask;
                byte_simd_index                           = 0;
            }
        }
    }

    _first_cpuarch_value = *view_as<uintptr_t*>(&_first_known_values[0]);
    _first_cpuarch_mask_value = (_first_known_values.size()
                                 >= sizeof(uintptr_t)) ?
                                  std::numeric_limits<uintptr_t>::max() :
                                  (1u << _first_known_values.size()) - 1u;
    _first_cpuarch_value      = _first_cpuarch_value
                           & _first_cpuarch_mask_value;

    /* Second scanning method using masks */
    {
        auto pattern_size = _values.size();
        simd_value_t smid_value {};
        size_t index_of_smid_value {};

        for (size_t index = 0; index < pattern_size;)
        {
            auto pattern_value = _values[index]->value;

            if (pattern_value == Value::UNKNOWN)
            {
                if (index_of_smid_value != 0)
                {
                    auto bytes_mask = index_of_smid_value;

#if (defined(__AVX512F__) || defined(__AVX2__))                          \
  && defined(PATTERN_UNALIGNED_SIMD_V1_V2)
    #ifdef __AVX512F__
                    std::array<int64_t, 8> e {};
    #elif defined(__AVX2__)
                    std::array<int64_t, 4> e {};
    #endif
                    size_t ei = 0;

                    if (bytes_mask >= sizeof(int64_t))
                    {
                        for (; ei < e.size()
                               && bytes_mask >= sizeof(int64_t);
                             ei++)
                        {
                            *view_as<uint64_t*>(&e[ei]) = std::
                              numeric_limits<uint64_t>::max();
                            bytes_mask -= sizeof(int64_t);
                        }

                        if (bytes_mask > 0)
                        {
                            *view_as<uint64_t*>(&e[ei]) = (1ull
                                                           << (bytes_mask
                                                               * CHAR_BIT))
                                                          - 1ull;
                        }
                    }
                    else
                    {
                        *view_as<uint64_t*>(&e[ei]) = (1ull
                                                       << (bytes_mask
                                                           * CHAR_BIT))
                                                      - 1ull;
                    }
#endif

#if defined(__AVX512F__) && defined(PATTERN_UNALIGNED_SIMD_V1_V2)
                    auto mask = _mm512_set_epi64(e[7],
                                                 e[6],
                                                 e[5],
                                                 e[4],
                                                 e[3],
                                                 e[2],
                                                 e[1],
                                                 e[0]);
#elif defined(__AVX2__) && defined(PATTERN_UNALIGNED_SIMD_V1_V2)
                    auto mask = _mm256_set_epi64x(e[3], e[2], e[1], e[0]);

#else
                    simd_value_t mask = (1ull << bytes_mask * CHAR_BIT)
                                        - 1ull;
#endif

                    _fast_values.push_back(
                      { 0, index_of_smid_value, smid_value, mask });

                    index_of_smid_value = 0;
                    std::memset(&smid_value, 0, sizeof(smid_value));
                }

                /* We got one already */
                index++;
                index_of_smid_value++;

                /* Search for the next ones, if there is */
                for (; index < pattern_size;
                     index++, index_of_smid_value++)
                {
                    pattern_value = _values[index]->value;

                    if (pattern_value != Value::UNKNOWN)
                    {
                        break;
                    }
                }

                fast_value_t fvalue { -1, index_of_smid_value, {}, {} };
                _fast_values.push_back(fvalue);

                index_of_smid_value = 0;
            }
            else
            {
                *view_as<byte_t*>(view_as<uintptr_t>(&smid_value) + index_of_smid_value) = view_as<
                  byte_t>(pattern_value);

                index_of_smid_value++;

                if (index_of_smid_value >= sizeof(simd_value_t))
                {
#if (defined(__AVX512F__) || defined(__AVX2__))                          \
  && defined(PATTERN_UNALIGNED_SIMD_V1_V2)
                    simd_value_t mask {};
                    std::memset(&mask, 0xFF, sizeof(simd_value_t));
#else
                    simd_value_t mask = std::numeric_limits<
                      uint64_t>::max();
#endif

                    _fast_values.push_back(
                      { 0, sizeof(simd_value_t), smid_value, mask });

                    index_of_smid_value = 0;
                    std::memset(&smid_value, 0, sizeof(smid_value));
                }

                index++;
            }
        }

        if (index_of_smid_value != 0)
        {
            auto bytes_mask = index_of_smid_value;

#if (defined(__AVX512F__) || defined(__AVX2__))                          \
  && defined(PATTERN_UNALIGNED_SIMD_V1_V2)
    #if defined(__AVX512F__)
            std::array<int64_t, 8> e {};
    #elif defined(__AVX2__)
            std::array<int64_t, 4> e {};
    #endif
            size_t ei = 0;

            if (bytes_mask >= sizeof(int64_t))
            {
                for (; ei < e.size() && bytes_mask >= sizeof(int64_t);
                     ei++)
                {
                    *view_as<uint64_t*>(&e[ei]) = std::numeric_limits<
                      uint64_t>::max();
                    bytes_mask -= sizeof(int64_t);
                }

                if (bytes_mask > 0)
                {
                    *view_as<uint64_t*>(&e[ei]) = (1ull << (bytes_mask
                                                            * CHAR_BIT))
                                                  - 1ull;
                }
            }
            else
            {
                *view_as<uint64_t*>(&e[ei]) = (1ull
                                               << (bytes_mask * CHAR_BIT))
                                              - 1ull;
            }
#endif

#if defined(__AVX512F__) && defined(PATTERN_UNALIGNED_SIMD_V1_V2)
            auto mask = _mm512_set_epi64(e[7],
                                         e[6],
                                         e[5],
                                         e[4],
                                         e[3],
                                         e[2],
                                         e[1],
                                         e[0]);
#elif defined(__AVX2__) && defined(PATTERN_UNALIGNED_SIMD_V1_V2)
            auto mask = _mm256_set_epi64x(e[3], e[2], e[1], e[0]);

#else
            simd_value_t mask = (1ull << bytes_mask * CHAR_BIT) - 1ull;
#endif
            _fast_values.push_back(
              { 0, index_of_smid_value, smid_value, mask });
        }
    }
}

XKLib::PatternByte::~PatternByte()
{
    std::free(_simd_values);
    std::free(_fast_masks);
}

auto XKLib::PatternByte::values() -> std::vector<std::shared_ptr<Value>>&
{
    return _values;
}

auto XKLib::PatternByte::fvalues() -> std::vector<fast_value_t>&
{
    return _fast_values;
}

auto XKLib::PatternByte::unknown_values() -> std::vector<unknown_value_t>&
{
    return _unknown_values;
}

auto XKLib::PatternByte::simd_values() -> simd_value_t*
{
    return _simd_values;
}

auto XKLib::PatternByte::simd_values_size() -> size_t&
{
    return _simd_values_size;
}

auto XKLib::PatternByte::matches() -> std::vector<ptr_t>&
{
    return _matches;
}

auto XKLib::PatternByte::isValid() -> bool
{
    if (_values.size() == 0)
    {
        return false;
    }

    for (auto&& byte : _values)
    {
        if (byte->value == Value::INVALID)
        {
            return false;
        }
    }

    /* ? xx xx ... */
    if (_values[0]->value == Value::UNKNOWN)
    {
        return false;
    }

    /* xx xx ? */
    if (_values[_values.size() - 1]->value == Value::UNKNOWN)
    {
        return false;
    }

    return true;
}

auto XKLib::PatternByte::scan(Process& process) -> void
{
    PatternScanning::searchInProcess(*this, process);
}

auto XKLib::PatternByte::areaName() -> std::string
{
    return _area_name;
}

auto XKLib::PatternByte::fast_masks() -> simd_value_t*
{
    return _fast_masks;
}

auto XKLib::PatternByte::first_known_values() -> std::vector<byte_t>&
{
    return _first_known_values;
}

uintptr_t XKLib::PatternByte::first_cpuarch_value()
{
    return _first_cpuarch_value;
}

uintptr_t XKLib::PatternByte::first_cpuarch_mask_value()
{
    return _first_cpuarch_mask_value;
}
