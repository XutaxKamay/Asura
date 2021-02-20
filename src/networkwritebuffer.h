#ifndef NETWORKWRITEBUFFER_H
#define NETWORKWRITEBUFFER_H

#include "buffer.h"
#include "exception.h"

namespace XLib
{
    template <safesize_t max_size_T = 0>
    class NetworkWriteBuffer : public Buffer<max_size_T>
    {
      public:
        NetworkWriteBuffer();

        NetworkWriteBuffer(data_t data,
                           bool allocated         = false,
                           safesize_t writtenBits = 0,
                           safesize_t maxSize     = 0);

        ~NetworkWriteBuffer() = default;

        auto spaceLeft()
        {
            return _written_bits < this->maxSize() * 8;
        }

        template <bool value>
        constexpr auto writeOneBit()
        {
            if (!spaceLeft())
            {
                throw XLIB_EXCEPTION("No space left");
            }

            WriteBit<value>(this->data(), _written_bits++);
        }

        auto writeOneBit(bool value);

        template <bool... values>
        constexpr auto writeBits()
        {
            writeOneBit<values...>();
        }

        template <typename... args_T>
        auto writeBits(args_T... args)
        {
            writeOneBit(args...);
        }

        auto writeBits(std::vector<bool>) -> void;

        template <typesize_t typesize_T>
        constexpr auto write(g_v_t<typesize_T> var, safesize_t size = 0)
        {
            safesize_t bits_to_write {};

            if constexpr (typesize_T == type_array)
            {
                bits_to_write = size * 8;

                for (safesize_t i = 0; i < bits_to_write; i++)
                {
                    writeOneBit(ReadBit(var, i));
                }
            }
            else
            {
                bits_to_write = sizeof(g_v_t<typesize_T>) * 8;

                for (safesize_t i = 0; i < bits_to_write; i++)
                {
                    writeOneBit(ReadBit(&var, i));
                }
            }
        }

        auto pos(safesize_t toBit = -1) -> safesize_t;

      private:
        safesize_t _written_bits {};
    };

    template <safesize_t max_size_T>
    NetworkWriteBuffer<max_size_T>::NetworkWriteBuffer()
     : Buffer<max_size_T>()
    {
    }

    template <safesize_t max_size_T>
    NetworkWriteBuffer<max_size_T>::NetworkWriteBuffer(
      data_t data,
      bool allocated,
      safesize_t writtenBits,
      safesize_t maxSize)
     : Buffer<max_size_T>(data, allocated, maxSize),
       _written_bits(writtenBits)
    {
    }

    template <safesize_t max_size_T>
    auto NetworkWriteBuffer<max_size_T>::writeOneBit(bool value)
    {
        if (!spaceLeft())
        {
            throw XLIB_EXCEPTION("No space left");
        }

        WriteBit(this->data(), _written_bits++, value);
    }

    template <safesize_t max_size_T>
    auto NetworkWriteBuffer<max_size_T>::writeBits(std::vector<bool> bools)
      -> void
    {
        for (auto&& b : bools)
        {
            writeOneBit(b);
        }
    }

    template <safesize_t max_size_T>
    auto NetworkWriteBuffer<max_size_T>::pos(safesize_t toBit)
      -> safesize_t
    {
        auto backup = _written_bits;

        if (toBit != -1)
        {
            _written_bits = toBit;
        }

        return backup;
    }

} // namespace XLib

#endif
