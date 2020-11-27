#ifndef NETWORKREADBUFFER_H
#define NETWORKREADBUFFER_H

#include "buffer.h"

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
                          bool allocated         = false,
                          safesize_t writtenBits = 0,
                          safesize_t maxSize     = 0);

        ~NetworkReadBuffer() = default;

        auto spaceLeft()
        {
            return _read_bits / 8 < this->maxSize();
        }

      public:
      private:
        /**
         * @brief _read_bits
         */
        safesize_t _read_bits {};
    };

    template <XLib::safesize_t max_size_T>
    NetworkReadBuffer<max_size_T>::NetworkReadBuffer()
    {
    }

}

#endif
