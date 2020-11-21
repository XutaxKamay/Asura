#ifndef BITS_H
#define BITS_H

#include <bitset>
#include "types.h"

namespace XLib
{
    template <safesize_t pos, typename T = data_t>
    constexpr auto ReadBit(T data)
    {
        constexpr auto real_pos = pos - 1;

        if constexpr (real_pos >= std::numeric_limits<safesize_t>::max())
        {
            static_assert("Can't read bit higher");
        }
        else if constexpr (real_pos <= 0)
        {
            static_assert("Can't read bit lower");
        }

        constexpr auto read_byte_pos = real_pos / 8;

        constexpr auto wanted_bit_value = (1 << (real_pos % 8));

        auto byte_value = *view_as<byte_t*>(view_as<uintptr_t>(data)
                                            + read_byte_pos);

        return view_as<bool>(byte_value & wanted_bit_value);
    }

    template <safesize_t pos, bool val, typename T = data_t>
    constexpr auto WriteBit(T data)
    {
        constexpr auto real_pos = pos - 1;

        if constexpr (real_pos >= std::numeric_limits<safesize_t>::max())
        {
            static_assert("Can't write bit higher");
        }
        else if constexpr (real_pos <= 0)
        {
            static_assert("Can't write bit lower");
        }

        constexpr auto read_byte_pos = real_pos / 8;

        constexpr auto wanted_bit_value = (1 << (real_pos % 8));

        auto byte_value = view_as<byte_t*>(view_as<uintptr_t>(data)
                                           + read_byte_pos);

        if constexpr (val)
        {
            *byte_value |= wanted_bit_value;
        }
        else
        {
            *byte_value &= ~wanted_bit_value;
        }
    }

    template <typename T = data_t>
    constexpr auto ReadBit(T data, safesize_t pos)
    {
        auto real_pos = pos - 1;

        if (real_pos >= std::numeric_limits<safesize_t>::max())
        {
            throw("Can't read bit higher");
        }
        else if (real_pos <= 0)
        {
            throw("Can't read bit lower");
        }

        auto read_byte_pos = real_pos / 8;

        auto wanted_bit_value = (1 << (real_pos % 8));

        auto byte_value = *view_as<byte_t*>(view_as<uintptr_t>(data)
                                            + read_byte_pos);

        return view_as<bool>(byte_value & wanted_bit_value);
    }

    template <bool val, typename T = data_t>
    constexpr auto WriteBit(T data, safesize_t pos)
    {
        auto real_pos = pos - 1;

        if (real_pos >= std::numeric_limits<safesize_t>::max())
        {
            throw("Can't write bit higher");
        }
        else if (real_pos <= 0)
        {
            throw("Can't write bit lower");
        }

        auto read_byte_pos = real_pos / 8;

        auto wanted_bit_value = (1 << (real_pos % 8));

        auto byte_value = view_as<byte_t*>(view_as<uintptr_t>(data)
                                           + read_byte_pos);

        if constexpr (val)
        {
            *byte_value |= wanted_bit_value;
        }
        else
        {
            *byte_value &= ~wanted_bit_value;
        }
    }

    template <typename T = data_t>
    constexpr auto WriteBit(T data, safesize_t pos, bool val)
    {
        auto real_pos = pos - 1;

        if (real_pos >= std::numeric_limits<safesize_t>::max())
        {
            throw("Can't write bit higher");
        }
        else if (real_pos <= 0)
        {
            throw("Can't write bit lower");
        }

        auto read_byte_pos = real_pos / 8;

        auto wanted_bit_value = (1 << (real_pos % 8));

        auto byte_value = view_as<byte_t*>(view_as<uintptr_t>(data)
                                           + read_byte_pos);

        if (val)
        {
            *byte_value |= wanted_bit_value;
        }
        else
        {
            *byte_value &= ~wanted_bit_value;
        }
    }
};

#endif
