#include "xkomprexk.h"
#include "networkreadbuffer.h"
#include "networkwritebuffer.h"

#include <algorithm>
#include <cmath>

XLib::XKomprexk::Exception::Exception(const std::string& msg) : _msg(msg)
{
}

const std::string& XLib::XKomprexk::Exception::msg()
{
    return _msg;
}

XLib::XKomprexk::XKomprexk(XLib::data_t data, size_t size)
 : _data(data), _size(size)
{
}

auto XLib::XKomprexk::decompress() -> XLib::bytes_t
{
    bytes_t result;

    _alphabet.clear();

    size_t read_size = 0;

    auto max_bits = *view_as<uint16_t*>(view_as<uintptr_t>(_data)
                                        + read_size);
    read_size += sizeof(uint16_t);

    auto max_freq_bits = *view_as<uint16_t*>(view_as<uintptr_t>(_data)
                                             + read_size);
    read_size += sizeof(uint16_t);

    auto alphabet_max_size = *view_as<uint16_t*>(view_as<uintptr_t>(_data)
                                                 + read_size);
    read_size += sizeof(uint16_t);

    for (uint16_t i = 0; i < alphabet_max_size; i++)
    {
        _alphabet.push_back(_data[read_size]);
        read_size++;
    }

    size_t read_bits = 0;

    std::vector<freq_t> freqs;

    while (read_size < _size)
    {
        freq_t freq;
        freq.byte  = 0;
        freq.count = 0;

        auto checkByte = [&read_size, &read_bits]()
        {
            read_bits++;

            if (read_bits == 8)
            {
                read_bits = 0;
                read_size++;
            }
        };

        auto bits_to_read = view_as<size_t>(max_freq_bits + max_bits);
        auto bytes_left   = _size - read_size;

        if (bits_to_read > (8 - read_bits) && bytes_left <= 1)
        {
            break;
        }

        for (byte_t i = 0; i < max_freq_bits; i++)
        {
            if (_data[read_size] & (1 << read_bits))
            {
                freq.count |= (1 << i);
            }

            checkByte();
        }

        for (byte_t i = 0; i < max_bits; i++)
        {
            if (_data[read_size] & (1 << read_bits))
            {
                freq.byte |= (1 << i);
            }

            checkByte();
        }

        freq.byte = _alphabet[freq.byte];

        freqs.push_back(freq);
    }

    for (auto&& freq : freqs)
    {
        for (size_t i = 0; i < freq.count; i++)
        {
            result.push_back(freq.byte);
        }
    }

    return result;
}

auto XLib::XKomprexk::compress() -> XLib::bytes_t
{
    bytes_t result;

    std::vector<freq_t> freqs;

    /* Construct the alphabet and prepare the data */
    _alphabet.clear();

    size_t i = 0;

    while (i < _size)
    {
        _alphabet.push_back(_data[i]);

        size_t start = i;

        freq_t freq;
        freq.byte  = _data[start];
        freq.count = 1;

        for (i++; i < _size; i++)
        {
            if (freq.count
                == std::numeric_limits<decltype(freq_t::count)>::max())
            {
                break;
            }

            if (_data[start] != _data[i])
            {
                break;
            }

            freq.count++;
        }

        freqs.push_back(freq);
    }

    std::sort(_alphabet.begin(), _alphabet.end());
    _alphabet.erase(std::unique(_alphabet.begin(), _alphabet.end()),
                    _alphabet.end());

    auto freq_count = freqs[0].count;

    for (size_t i = 1; i < freqs.size(); i++)
    {
        if (freq_count < freqs[i].count)
        {
            freq_count = freqs[i].count;
        }
    }

    /* Get the alphabet max size */
    auto alphabet_max_size = view_as<uint16_t>(_alphabet.size());

    /* Get the number of maximum bits, shouldn't go above 8 bits */
    auto max_bits = view_as<uint16_t>(std::log2(alphabet_max_size - 1))
                    + 1;
    auto max_freq_bits = view_as<uint16_t>(std::log2(freq_count)) + 1;

    for (size_t i = 0; i < sizeof(uint16_t) * 3; i++)
    {
        result.push_back(0);
    }

    size_t write_size = 0;
    *view_as<uint16_t*>(view_as<uintptr_t>(result.data()) + write_size) = max_bits;
    write_size += sizeof(uint16_t);

    *view_as<uint16_t*>(view_as<uintptr_t>(result.data()) + write_size) = max_freq_bits;
    write_size += sizeof(uint16_t);

    *view_as<uint16_t*>(view_as<uintptr_t>(result.data()) + write_size) = alphabet_max_size;
    write_size += sizeof(uint16_t);

    std::vector<byte_t> pos(std::pow<size_t>(2, 8 * sizeof(byte_t)));

    size_t letter_pos = 0;

    for (auto&& letter : _alphabet)
    {
        result.push_back(letter);
        pos[letter] = letter_pos;

        letter_pos++;
    }

    byte_t result_byte                 = 0;
    size_t written_bits_on_result_byte = 0;

    for (auto&& freq : freqs)
    {
        auto checkByte =
          [&written_bits_on_result_byte, &result, &result_byte]()
        {
            written_bits_on_result_byte++;

            if (written_bits_on_result_byte == 8)
            {
                result.push_back(result_byte);
                written_bits_on_result_byte = 0;
                result_byte                 = 0;
            }
        };

        for (byte_t i = 0; i < max_freq_bits; i++)
        {
            if (freq.count & (1 << i))
            {
                result_byte |= (1 << written_bits_on_result_byte);
            }

            checkByte();
        }

        auto pos_alphabet = pos[freq.byte];

        for (byte_t i = 0; i < max_bits; i++)
        {
            if (pos_alphabet & (1 << i))
            {
                result_byte |= (1 << written_bits_on_result_byte);
            }

            checkByte();
        }
    }

    if (written_bits_on_result_byte > 0)
    {
        result.push_back(result_byte);
    }

    return result;
}
