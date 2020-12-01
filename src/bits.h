#ifndef BITS_H
#define BITS_H

#include <bitset>

#include "bufferexception.h"
#include "types.h"

namespace XLib
{
    template <safesize_t pos, typename T = data_t>
    constexpr auto ReadBit(T data)
    {
        if constexpr (pos >= std::numeric_limits<safesize_t>::max())
        {
            static_assert("Can't read bit higher");
        }
        else if constexpr (pos < 0)
        {
            static_assert("Can't read bit lower");
        }

        constexpr auto read_byte_pos = pos / 8;

        constexpr auto wanted_bit_value = (1 << (pos % 8));

        auto byte_value = *view_as<byte_t*>(view_as<uintptr_t>(data)
                                            + read_byte_pos);

        return view_as<bool>(byte_value & wanted_bit_value);
    }

    template <safesize_t pos, bool val, typename T = data_t>
    constexpr auto WriteBit(T data)
    {
        if constexpr (pos >= std::numeric_limits<safesize_t>::max())
        {
            static_assert("Can't write bit higher");
        }
        else if constexpr (pos < 0)
        {
            static_assert("Can't write bit lower");
        }

        constexpr auto read_byte_pos = pos / 8;

        constexpr auto wanted_bit_value = (1 << (pos % 8));

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
        if (pos >= std::numeric_limits<safesize_t>::max())
        {
            throw BufferException(std::string(CURRENT_CONTEXT)
                                  + "Can't read bit higher");
        }
        else if (pos < 0)
        {
            throw BufferException(std::string(CURRENT_CONTEXT)
                                  + "Can't read bit lower");
        }

        auto read_byte_pos = pos / 8;

        auto wanted_bit_value = (1 << (pos % 8));

        auto byte_value = *view_as<byte_t*>(view_as<uintptr_t>(data)
                                            + read_byte_pos);

        return view_as<bool>(byte_value & wanted_bit_value);
    }

    template <bool val, typename T = data_t>
    constexpr auto WriteBit(T data, safesize_t pos)
    {
        if (pos >= std::numeric_limits<safesize_t>::max())
        {
            throw BufferException(std::string(CURRENT_CONTEXT)
                                  + "Can't write bit higher");
        }
        else if (pos < 0)
        {
            throw BufferException(std::string(CURRENT_CONTEXT)
                                  + "Can't write bit lower");
        }

        auto read_byte_pos = pos / 8;

        auto wanted_bit_value = (1 << (pos % 8));

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
        if (pos >= std::numeric_limits<safesize_t>::max())
        {
            throw BufferException(std::string(CURRENT_CONTEXT)
                                  + "Can't write bit higher");
        }
        else if (pos < 0)
        {
            throw BufferException(std::string(CURRENT_CONTEXT)
                                  + "Can't write bit lower");
        }

        auto read_byte_pos = pos / 8;

        auto wanted_bit_value = (1 << (pos % 8));

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
