#ifndef NETWORKWRITEBUFFER_H
#define NETWORKWRITEBUFFER_H

#include "buffer.h"
#include "bufferexception.h"
#include <vector>

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
            return _written_bits / 8 < this->maxSize();
        }

        template <bool value>
        constexpr auto writeOneBit()
        {
            if (!spaceLeft())
            {
                throw BufferException(std::string(CURRENT_CONTEXT)
                                      + "No space left");
            }

            WriteBit<value>(this->data(), _written_bits++);
        }

        auto writeOneBit(bool value);

        template <bool... values>
        constexpr auto writeBits()
        {
            if (!spaceLeft())
            {
                throw BufferException(std::string(CURRENT_CONTEXT)
                                      + "No space left");
            }

            WriteBit<values...>(this->data(), _written_bits++);
        }

        template <typename... args_T>
        auto writeBits(args_T... args)
        {
            if constexpr (!std::is_same<args_T..., bool>::value)
                static_assert("One of these are not booleans, check"
                              "your code");

            if (!spaceLeft())
            {
                throw BufferException(std::string(CURRENT_CONTEXT)
                                      + "No space left");
            }

            WriteBit(this->data(), _written_bits++, args...);
        }

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
            throw BufferException(std::string(CURRENT_CONTEXT)
                                  + "No space left");
        }

        WriteBit(this->data(), _written_bits++, value);
    }

} // namespace XLib

#endif
