#ifndef BITS_H
#define BITS_H

#include "exception.h"
#include "types.h"

namespace XKLib
{
    template <const std::size_t P>
    constexpr auto read_bit(const auto data)
    {
        constexpr auto read_byte_pos = P / CHAR_BIT;

        constexpr auto wanted_bit_value = (1u << (P % CHAR_BIT));

        const auto byte_value = *view_as<byte_t*>(
          view_as<std::uintptr_t>(data) + read_byte_pos);

        return view_as<bool>(byte_value & wanted_bit_value);
    }

    template <std::size_t P, bool V>
    constexpr auto write_bit(const auto data)
    {
        constexpr auto read_byte_pos = P / CHAR_BIT;

        constexpr auto wanted_bit_value = (1u << (P % CHAR_BIT));

        const auto byte_value = view_as<byte_t*>(
          view_as<std::uintptr_t>(data) + read_byte_pos);

        if constexpr (V)
        {
            *byte_value |= wanted_bit_value;
        }
        else
        {
            *byte_value &= ~wanted_bit_value;
        }
    }

    constexpr auto read_bit(const auto data, const std::size_t pos)
    {
        const auto read_byte_pos = pos / CHAR_BIT;

        const auto wanted_bit_value = (1u << (pos % CHAR_BIT));

        const auto byte_value = *view_as<byte_t*>(
          view_as<std::uintptr_t>(data) + read_byte_pos);

        return view_as<bool>(byte_value & wanted_bit_value);
    }

    template <bool V>
    constexpr auto write_bit(const auto data, const std::size_t P)
    {
        const auto read_byte_pos = P / CHAR_BIT;

        const auto wanted_bit_value = (1u << (P % CHAR_BIT));

        const auto byte_value = view_as<byte_t*>(
          view_as<std::uintptr_t>(data) + read_byte_pos);

        if constexpr (V)
        {
            *byte_value |= wanted_bit_value;
        }
        else
        {
            *byte_value &= ~wanted_bit_value;
        }
    }

    constexpr auto write_bit(const auto data,
                             const std::size_t pos,
                             const bool val)
    {
        const auto read_byte_pos = pos / CHAR_BIT;

        const auto wanted_bit_value = (1u << (pos % CHAR_BIT));

        const auto byte_value = view_as<byte_t*>(
          view_as<std::uintptr_t>(data) + read_byte_pos);

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
    auto bits_to_int(const std::vector<bool>& bits)
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
    auto bits_to_int(const data_t data, const std::size_t nbBits = 1)
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

    auto int_to_bits(const auto val)
    {
        std::vector<bool> bits;

        for (std::size_t i = 0; i < sizeof(val) * CHAR_BIT; i++)
        {
            bits.push_back(read_bit(&val, i));
        }

        return bits;
    }

    auto bits_needed(const auto max_val) -> decltype(max_val)
    {
        if (max_val > 0)
        {
            return view_as<decltype(max_val)>(std::log2(max_val)) + 1;
        }

        return {};
    }
}

#endif
