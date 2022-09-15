#include "pch.h"

#include "networkreadbuffer.h"

Asura::NetworkReadBuffer::NetworkReadBuffer(const data_t data,
                                            const std::size_t maxSize,
                                            const std::size_t readBits)
 : Buffer(data, maxSize),
   _read_bits(readBits)
{
}

auto Asura::NetworkReadBuffer::readBit() -> bool
{
    if (_read_bits / CHAR_BIT >= maxSize())
    {
        ASURA_EXCEPTION("Filled buffer");
    }

    return read_bit(data(), _read_bits++);
}

void Asura::NetworkReadBuffer::pos(const std::size_t toBit)
{
    _read_bits = toBit;
}
