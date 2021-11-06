#include "pch.h"

#include "patternscanning.h"
#include "process.h"

XKLib::PatternByte::Value::Value(int value) : value(value)
{
}

XKLib::PatternByte::PatternByte(std::vector<std::shared_ptr<Value>> values,
                                std::string areaName,
                                std::vector<ptr_t> matches)
 : _values(std::move(values)), _matches(std::move(matches)),
   _area_name(std::move(areaName)), _fast_aligned_values_count(0)
{
    if (!isValid())
    {
        XKLIB_EXCEPTION("Invalid pattern.");
    }

    /**
     *  Let's do some preprocessing now for different scanning systems
     */

    _unknown_values.reserve(_values.size());

    /* index of the byte in the pattern */
    size_t index = 0;
    /* count unknown bytes that were contigous inside the pattern */
    size_t ukval_contigous_count = 0;
    /**
     * store the last time the unknown bytes were contigous for
     * pushing into a vector the unknown bytes
     */
    size_t index_since_contigous_count = 0;
    /* index of the simd value */
    size_t byte_simd_index = 0;
    /* count how many unknown bytes we got */
    size_t count_unknown_byte = 0;
    bool are_known_values     = true;
    std::vector<byte_t> known_values;

    /**
     * Allocate SIMD values here, we're not using std::vector but we
     * could. std::vector should be optimized, but I like this way.
     */
    _fast_aligned_masks = view_as<simd_value_t*>(
      std::aligned_alloc(sizeof(simd_value_t), _values.size()));
    _fast_aligned_values = view_as<simd_value_t*>(
      std::aligned_alloc(sizeof(simd_value_t), _values.size()));
    simd_value_t simd_value {}, simd_mask {};

    for (auto&& value : _values)
    {
        value->index = index;

        if (value->value == Value::UNKNOWN)
        {
            index_since_contigous_count = index;
            ukval_contigous_count++;

            /* do we got a full simd integer here */
            if (ukval_contigous_count >= sizeof(simd_value_t))
            {
                /**
                 * store info about what index would be the simd
                 * value, etc ... */
                _unknown_values.push_back(
                  { index_since_contigous_count / sizeof(simd_value_t),
                    index_since_contigous_count % sizeof(simd_value_t),
                    index_since_contigous_count,
                    ukval_contigous_count });
                ukval_contigous_count = 0;
            }

            /* push back the last known values */
            if (are_known_values)
            {
                _vec_known_values.push_back(
                  std::make_shared<decltype(known_values)>(known_values));
                known_values.clear();
                are_known_values = false;
            }

            count_unknown_byte++;
        }
        else
        {
            if (!are_known_values)
            {
                /* push back the last count of unknown_bytes */
                _vec_skipper_uk_values.push_back(count_unknown_byte);
                count_unknown_byte = 0;
                are_known_values   = true;
            }

            /* push back known value */
            known_values.push_back(view_as<byte_t>(value->value));

            /* check if previous unknown bytes weren't a full simd
             * value and push it back */
            if (ukval_contigous_count > 0)
            {
                _unknown_values.push_back(
                  { index_since_contigous_count / sizeof(simd_value_t),
                    index_since_contigous_count % sizeof(simd_value_t),
                    index_since_contigous_count,
                    ukval_contigous_count });
                ukval_contigous_count = 0;
            }

            /**
             * copy pattern data to the simd value and create the
             * mask
             */
            view_as<byte_t*>(&simd_value)[byte_simd_index] = view_as<
              byte_t>(value->value);
            view_as<byte_t*>(&simd_mask)[byte_simd_index] = 0xFF;
        }

        index++;
        byte_simd_index++;

        /* do wo we got a full simd value here */
        if (byte_simd_index >= sizeof(simd_value_t))
        {
            /**
             * if yes we store the mask & the value inside the fast
             * aligned vectors with SIMD instructions
             */
#if defined(__AVX512F__)
            _mm512_store_si512(
              &_fast_aligned_values[_fast_aligned_values_count],
              simd_value);
            _mm512_store_si512(
              &_fast_aligned_masks[_fast_aligned_values_count],
              simd_mask);
#elif defined(__AVX2__)
            _mm256_store_si256(
              &_fast_aligned_values[_fast_aligned_values_count],
              simd_value);
            _mm256_store_si256(
              &_fast_aligned_masks[_fast_aligned_values_count],
              simd_mask);
#else
            _fast_aligned_values[_fast_aligned_values_count] = simd_value;
            _fast_aligned_masks[_fast_aligned_values_count]  = simd_mask;
#endif
            /* reset values */
            std::memset(&simd_value, 0, sizeof(simd_value));
            std::memset(&simd_mask, 0, sizeof(simd_mask));
            byte_simd_index = 0;

            /* increment counter */
            _fast_aligned_values_count++;
        }
    }

    /* was there still some known values left after an unknown byte */
    if (known_values.size())
    {
        /* if yes push it back */
        _vec_known_values.push_back(
          std::make_shared<decltype(known_values)>(known_values));
    }

    /**
     * was there some left bytes that needs to be stored a simd value
     */
    if (byte_simd_index > 0)
    {
        /* if yes push it back */
#if defined(__AVX512F__)
        _mm512_store_si512(
          &_fast_aligned_values[_fast_aligned_values_count],
          simd_value);
        _mm512_store_si512(
          &_fast_aligned_masks[_fast_aligned_values_count],
          simd_mask);
#elif defined(__AVX2__)
        _mm256_store_si256(
          &_fast_aligned_values[_fast_aligned_values_count],
          simd_value);
        _mm256_store_si256(
          &_fast_aligned_masks[_fast_aligned_values_count],
          simd_mask);
#else
        _fast_aligned_values[_fast_aligned_values_count] = simd_value;
        _fast_aligned_masks[_fast_aligned_values_count] = simd_mask;
#endif
        /* don't forget to increment */
        _fast_aligned_values_count++;
    }

    if (_vec_skipper_uk_values.size() != (_vec_known_values.size() - 1))
    {
        XKLIB_EXCEPTION("The amount of vec known values should be the "
                        "same of vec of unknown values");
    }
}

XKLib::PatternByte::~PatternByte()
{
    std::free(_fast_aligned_values);
    std::free(_fast_aligned_masks);
}

auto XKLib::PatternByte::values() -> std::vector<std::shared_ptr<Value>>&
{
    return _values;
}

auto XKLib::PatternByte::simd_unknown_values()
  -> std::vector<simd_unknown_value_t>&
{
    return _unknown_values;
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

auto XKLib::PatternByte::vec_known_values()
  -> std::vector<std::shared_ptr<std::vector<byte_t>>>&
{
    return _vec_known_values;
}

auto XKLib::PatternByte::vec_skipper_uk_values() -> std::vector<size_t>&
{
    return _vec_skipper_uk_values;
}

auto XKLib::PatternByte::fast_aligned_values() -> simd_value_t*
{
    return _fast_aligned_values;
}

auto XKLib::PatternByte::fast_aligned_masks() -> simd_value_t*
{
    return _fast_aligned_masks;
}

auto XKLib::PatternByte::fast_aligned_values_count() -> size_t
{
    return _fast_aligned_values_count;
}
