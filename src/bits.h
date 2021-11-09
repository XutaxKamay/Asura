#ifndef BITS_H
#define BITS_H

#include "exception.h"
#include "types.h"

namespace XKLib
{
    template <std::size_t pos, typename T = data_t>
    constexpr auto read_bit(T data)
    {
        constexpr auto read_byte_pos = pos / CHAR_BIT;

        constexpr auto wanted_bit_value = (1u << (pos % CHAR_BIT));

        auto byte_value = *view_as<byte_t*>(view_as<uintptr_t>(data)
                                            + read_byte_pos);

        return view_as<bool>(byte_value & wanted_bit_value);
    }

    template <std::size_t pos, bool val, typename T = data_t>
    constexpr auto write_bit(T data)
    {
        constexpr auto read_byte_pos = pos / CHAR_BIT;

        constexpr auto wanted_bit_value = (1u << (pos % CHAR_BIT));

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
    constexpr auto read_bit(T data, std::size_t pos)
    {
        auto read_byte_pos = pos / CHAR_BIT;

        auto wanted_bit_value = (1u << (pos % CHAR_BIT));

        auto byte_value = *view_as<byte_t*>(view_as<uintptr_t>(data)
                                            + read_byte_pos);

        return view_as<bool>(byte_value & wanted_bit_value);
    }

    template <bool val, typename T = data_t>
    constexpr auto write_bit(T data, std::size_t pos)
    {
        auto read_byte_pos = pos / CHAR_BIT;

        auto wanted_bit_value = (1u << (pos % CHAR_BIT));

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
    constexpr auto write_bit(T data, std::size_t pos, bool val)
    {
        auto read_byte_pos = pos / CHAR_BIT;

        auto wanted_bit_value = (1u << (pos % CHAR_BIT));

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

    template <typename T>
    auto bits_to_int(std::vector<bool> bits)
    {
        T var {};

        for (std::size_t i = 0; i < bits.size(); i++)
        {
            if (bits[i])
            {
                var += view_as<T>(1) << view_as<T>(i);
            }
        }

        return var;
    }

    template <typename T>
    auto bits_to_int(data_t data, std::size_t nbBits = 1)
    {
        T var {};

        for (std::size_t i = 0; i < nbBits; i++)
        {
            if (read_bit(data, i))
            {
                var += view_as<T>(1) << view_as<T>(i);
            }
        }

        return var;
    }

    template <typename T>
    auto int_to_bits(T val)
    {
        std::vector<bool> bits;

        for (std::size_t i = 0; i < sizeof(T) * CHAR_BIT; i++)
        {
            bits.push_back(read_bit(&val, i));
        }

        return bits;
    }

    template <typename T>
    auto bits_needed(T max_val) -> T
    {
        if (max_val > 0)
        {
            return view_as<T>(std::log2(max_val)) + 1;
        }

        return {};
    };

};

#endif
