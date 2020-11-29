#ifndef NETWORKREADBUFFER_H
#define NETWORKREADBUFFER_H

#include "buffer.h"
#include "bufferexception.h"

namespace XLib
{
    template <safesize_t max_size_T = 0>
    /**
     * @brief The NetworkReadBuffer class
     * This class permits to read a buffer easily.
     * Example:
     * NetworkReadBuffer<1024> readBuffer;
     * auto b = readBuffer.readVar<type_8>();
     */
    class NetworkReadBuffer : public Buffer<max_size_T>
    {
      public:
        /**
         * @brief NetworkReadBuffer
         */
        NetworkReadBuffer();

        NetworkReadBuffer(data_t data,
                          bool allocated      = false,
                          safesize_t readBits = 0,
                          safesize_t maxSize  = 0);

        ~NetworkReadBuffer() = default;

        auto spaceLeft()
        {
            return _read_bits / 8 < this->maxSize();
        }

        template <safesize_t pos_T>
        constexpr auto readOneBit()
        {
            if (!spaceLeft())
            {
                throw BufferException(std::string(CURRENT_CONTEXT)
                                      + "No space left");
            }

            _read_bits++;
            return ReadBit<pos_T>(this->data());
        }

        auto readOneBit(safesize_t pos);

        template <safesize_t nb_of_bits_T>
        auto readBits()
        {
            std::vector<bool> bits;

            for (safesize_t i = 0; i < nb_of_bits_T; i++)
            {
                bits.push_back(readOneBit(i));
            }

            return bits;
        }

        auto readBits(safesize_t nbOfBits);

        template <typesize_t typesize_T>
        auto read(size_t size = 0, size_t* pReadSize = nullptr)
        {
            size_t bits_to_read {};

            if constexpr (typesize_T == type_array)
            {
                bits_to_read = size * 8;
            }
            else
            {
                bits_to_read = sizeof(g_v_t<typesize_T>) * 8;
            }

            auto bits = readBits(bits_to_read);

            if (pReadSize)
            {
                *pReadSize = bits.size() / 8;
            }

            if constexpr (typesize_T == type_array)
            {
                std::vector<byte_t> data;

                for (int i = 0; i < bits.size(); i += 8)
                {
                    byte_t byte = 0;

                    for (int j = 0; j < 8; j++)
                    {
                        byte += (1 << bits[j]);
                    }

                    data.push_back(byte);
                }

                return data;
            }
            else
            {
                g_v_t<typesize_T> var {};

                for (int i = 0; i < bits.size(); i++)
                {
                    var += (view_as<g_v_t<typesize_T>>(1)
                            << view_as<g_v_t<typesize_T>>(bits[i]));
                }

                return var;
            }
        }

      public:
      private:
        /**
         * @brief _read_bits
         */
        safesize_t _read_bits {};
    };

    template <safesize_t max_size_T>
    NetworkReadBuffer<max_size_T>::NetworkReadBuffer()
    {
    }

    template <safesize_t max_size_T>
    NetworkReadBuffer<max_size_T>::NetworkReadBuffer(data_t data,
                                                     bool allocated,
                                                     safesize_t readBits,
                                                     safesize_t maxSize)
     : Buffer<max_size_T>(data, allocated, maxSize), _read_bits(readBits)
    {
    }

    template <safesize_t max_size_T>
    auto NetworkReadBuffer<max_size_T>::readOneBit(safesize_t pos)
    {
        if (!spaceLeft())
        {
            throw BufferException(std::string(CURRENT_CONTEXT)
                                  + "No space left");
        }

        _read_bits++;

        return ReadBit(this->data(), pos);
    }

    template <safesize_t max_size_T>
    auto NetworkReadBuffer<max_size_T>::readBits(safesize_t nbOfBits)
    {
        std::vector<bool> bits;

        for (safesize_t i = 0; i < nbOfBits; i++)
        {
            bits.push_back(readOneBit(i));
        }

        return bits;
    }
}

#endif
