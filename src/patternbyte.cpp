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

        auto left = _values.size() % sizeof(simd_value_t);

        if (left > 0)
        {
            left = sizeof(simd_value_t) - left;
        }

        _simd_values_size = _values.size() + left;

        size_t index                       = 0;
        size_t ukval_contigous_count       = 0;
        size_t index_since_contigous_count = 0;

        for (auto&& value : _values)
        {
            value->index = index;

            if (value->value == Value::UNKNOWN)
            {
                index_since_contigous_count = index;
                ukval_contigous_count++;

                if (ukval_contigous_count >= sizeof(simd_value_t))
                {
                    _unknown_values.push_back(
                      { index_since_contigous_count / sizeof(simd_value_t),
                        index_since_contigous_count % sizeof(simd_value_t),
                        index_since_contigous_count,
                        ukval_contigous_count });
                    ukval_contigous_count = 0;
                }
            }
            else
            {
                if (ukval_contigous_count > 0)
                {
                    _unknown_values.push_back(
                      { index_since_contigous_count / sizeof(simd_value_t),
                        index_since_contigous_count % sizeof(simd_value_t),
                        index_since_contigous_count,
                        ukval_contigous_count });
                    ukval_contigous_count = 0;
                }

                view_as<byte_t*>(_simd_values)[index] = view_as<byte_t>(
                  value->value);
            }

            index++;
        }
    }

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

#if defined(__AVX512F__) || defined(__AVX2__)
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

#ifdef __AVX512F__
                    auto mask = _mm512_set_epi64(e[7],
                                                 e[6],
                                                 e[5],
                                                 e[4],
                                                 e[3],
                                                 e[2],
                                                 e[1],
                                                 e[0]);
#elif defined(__AVX2__)
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
#if defined(__AVX512F__) || defined(__AVX2__)
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

#if defined(__AVX512F__) || defined(__AVX2__)
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

#ifdef __AVX512F__
            auto mask = _mm512_set_epi64(e[7],
                                         e[6],
                                         e[5],
                                         e[4],
                                         e[3],
                                         e[2],
                                         e[1],
                                         e[0]);
#elif defined(__AVX2__)
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
