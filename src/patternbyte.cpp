#include "pch.h"

#include "patternscanning.h"
#include "process.h"

XKLib::PatternByte::Value::Value(int value) : value(value)
{
}

XKLib::PatternByte::PatternByte(std::vector<Value> values,
                                std::string areaName,
                                std::vector<ptr_t> matches)
 : _bytes(std::move(values)), _matches(std::move(matches)),
   _area_name(std::move(areaName))
{
    if (!isValid())
    {
        XKLIB_EXCEPTION("Invalid pattern.");
    }

    /**
     *  Let's do some preprocessing now for different scanning systems
     */

    _unknown_values.reserve(_bytes.size());

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
    /* organized values */
    bool are_known_values = true;
    std::vector<byte_t> known_bytes;

    /**
     * Allocate SIMD values here, we're not using std::vector but we
     * could. std::vector should be optimized, but I like this way.
     */
    simd_value_t simd_value {}, simd_mask {};

    for (auto&& byte : _bytes)
    {
        byte.index = index;

        if (byte.value == Value::UNKNOWN)
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

            if (are_known_values)
            {
                are_known_values = false;
            }

            count_unknown_byte++;
        }
        else
        {
            if (!are_known_values)
            {
                organized_simd_mv_t tmp_organized_mv {};
                size_t tmp_mv_byte_index = 0;
                std::vector<organized_simd_mv_t> aligned_fast_mvs;

                /**
                 * convert all unknown bytes to simd values
                 */

                for (auto&& known_byte : known_bytes)
                {
                    view_as<byte_t*>(&tmp_organized_mv.value)[tmp_mv_byte_index] = view_as<
                      byte_t>(known_byte);

                    view_as<byte_t*>(&tmp_organized_mv.mask)[tmp_mv_byte_index] = 0xFF;

                    tmp_mv_byte_index++;

                    if (tmp_mv_byte_index >= sizeof(simd_value_t))
                    {
                        tmp_organized_mv.size_to_move = sizeof(
                          simd_value_t);
                        aligned_fast_mvs.push_back(tmp_organized_mv);
                        std::memset(&tmp_organized_mv,
                                    0,
                                    sizeof(tmp_organized_mv));
                        tmp_mv_byte_index = 0;
                    }
                }

                if (tmp_mv_byte_index)
                {
                    tmp_organized_mv.size_to_move = tmp_mv_byte_index;
                    aligned_fast_mvs.push_back(tmp_organized_mv);
                }

                /* push back the last count of unknown_bytes */
                _vec_organized_values.push_back(
                  { known_bytes, aligned_fast_mvs, count_unknown_byte });
                count_unknown_byte = 0;
                known_bytes.clear();
                are_known_values = true;
            }

            /* push back known value */
            known_bytes.push_back(view_as<byte_t>(byte.value));

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
              byte_t>(byte.value);
            view_as<byte_t*>(&simd_mask)[byte_simd_index] = 0xFF;
        }

        index++;
        byte_simd_index++;

        /* do wo we got a full simd value here */
        if (byte_simd_index >= sizeof(simd_value_t))
        {
            _fast_aligned_mvs.push_back({ simd_mask, simd_value });

            /* reset values */
            std::memset(&simd_value, 0, sizeof(simd_value));
            std::memset(&simd_mask, 0, sizeof(simd_mask));
            byte_simd_index = 0;
        }
    }

    /* was there still some known values left after an unknown byte */
    if (known_bytes.size())
    {
        organized_simd_mv_t tmp_organized_mv {};
        size_t tmp_mv_byte_index = 0;
        std::vector<organized_simd_mv_t> aligned_fast_mvs;

        /**
         * convert all unknown bytes to simd values
         */

        for (auto&& known_byte : known_bytes)
        {
            view_as<byte_t*>(&tmp_organized_mv.value)[tmp_mv_byte_index] = view_as<
              byte_t>(known_byte);

            view_as<byte_t*>(&tmp_organized_mv.mask)[tmp_mv_byte_index] = 0xFF;

            tmp_mv_byte_index++;

            if (tmp_mv_byte_index >= sizeof(simd_value_t))
            {
                tmp_organized_mv.size_to_move = sizeof(simd_value_t);
                aligned_fast_mvs.push_back(tmp_organized_mv);
                std::memset(&tmp_organized_mv,
                            0,
                            sizeof(tmp_organized_mv));
                tmp_mv_byte_index = 0;
            }
        }

        if (tmp_mv_byte_index)
        {
            tmp_organized_mv.size_to_move = tmp_mv_byte_index;
            aligned_fast_mvs.push_back(tmp_organized_mv);
        }

        /* if yes push it back */
        _vec_organized_values.push_back(
          { known_bytes, aligned_fast_mvs, 0 });
    }

    /**
     * was there some left bytes that needs to be stored a simd value
     */
    if (byte_simd_index > 0)
    {
        _fast_aligned_mvs.push_back({ simd_mask, simd_value });
    }
}

auto XKLib::PatternByte::bytes() -> std::vector<Value>&
{
    return _bytes;
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
    if (_bytes.size() == 0)
    {
        return false;
    }

    for (auto&& byte : _bytes)
    {
        if (byte.value == Value::INVALID)
        {
            return false;
        }
    }

    /* ? xx xx ... */
    if (_bytes[0].value == Value::UNKNOWN)
    {
        return false;
    }

    /* xx xx ? */
    if (_bytes[_bytes.size() - 1].value == Value::UNKNOWN)
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

auto XKLib::PatternByte::vec_organized_values()
  -> std::vector<organized_values_t>&
{
    return _vec_organized_values;
}

auto XKLib::PatternByte::fast_aligned_mvs() -> std::vector<simd_mv_t>&
{
    return _fast_aligned_mvs;
}
