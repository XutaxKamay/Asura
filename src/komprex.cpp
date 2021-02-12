#include "komprex.h"
#include "networkreadbuffer.h"
#include "networkwritebuffer.h"

#include <algorithm>
#include <cmath>

XLib::Komprex::Exception::Exception(const std::string& msg) : _msg(msg)
{
}

const std::string& XLib::Komprex::Exception::msg()
{
    return _msg;
}

XLib::Komprex::Komprex(XLib::data_t data, size_t size)
 : _data(data), _size(size)
{
}

auto XLib::Komprex::decompress() -> XLib::bytes_t
{
    bytes_t result;
    return result;
}

auto XLib::Komprex::compress() -> XLib::bytes_t
{
    bytes_t result;

    std::vector<freq_t> freqs;

    /* Construct the alphabet and prepare the data */
    _alphabet.clear();

    for (size_t i = 0; i < _size; i++)
    {
        _alphabet.push_front(_data[i]);

        size_t start = i++;

        freq_t freq;
        freq.byte  = _data[start];
        freq.count = 1;

        for (; i < _size; i++)
        {
            if (_data[start] != _data[i]
                || freq.count
                     == std::numeric_limits<decltype(freq_t::count)>::max())
            {
                break;
            }

            freq.count++;
        }

        freqs.push_back(freq);
    }

    _alphabet.sort();
    _alphabet.unique();

    auto freq_count = freqs[0].count;

    for (size_t i = 1; i < freqs.size(); i++)
    {
        if (freq_count < freqs[i].count)
        {
            freq_count = freqs[i].count;
        }
    }

    /* Get the alphabet max size */
    auto alphabet_max_size = view_as<byte_t>(_alphabet.size());

    /* Get the number of maximum bits, shouldn't go above 8 bits */
    auto max_bits = view_as<byte_t>(std::log2(alphabet_max_size)) + 1;
    auto max_freq_bits = view_as<byte_t>(std::log2(freq_count)) + 1;

    if (max_bits > 8)
    {
        throw Exception(std::string(CURRENT_CONTEXT)
                        + "Komprex found more than 255 letters in "
                          "alphabet.");
    }

    if (max_freq_bits > 8)
    {
        throw Exception(std::string(CURRENT_CONTEXT)
                        + "Komprex can't encode the frequency integer "
                          "more than 8 bits");
    }

    size_t written_size = 0;

    result.push_back(max_bits);
    written_size++;

    result.push_back(max_freq_bits);
    written_size++;

    result.push_back(alphabet_max_size);
    written_size++;

    std::vector<size_t> pos(256);

    size_t letter_pos = 0;

    for (auto&& letter : _alphabet)
    {
        result.push_back(letter);
        written_size++;

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

            written_bits_on_result_byte++;
            checkByte();
        }

        for (byte_t i = 0; i < max_bits; i++)
        {
            if (pos[freq.byte] & (1 << i))
            {
                result_byte |= (1 << written_bits_on_result_byte);
            }

            written_bits_on_result_byte++;
            checkByte();
        }
    }

    return result;
}
