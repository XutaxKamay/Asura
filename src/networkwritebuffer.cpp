#include "pch.h"

#include "networkwritebuffer.h"

Asura::NetworkWriteBuffer::NetworkWriteBuffer(Asura::data_t data,
                                              std::size_t maxSize,
                                              std::size_t writtenBits)
 : Buffer(data, maxSize),
   _written_bits(writtenBits)
{
}

void Asura::NetworkWriteBuffer::writeBit(bool value)
{
    if (_written_bits / CHAR_BIT >= maxSize())
    {
        ASURA_EXCEPTION("Filled buffer");
    }

    write_bit(data(), _written_bits++, value);
}

void Asura::NetworkWriteBuffer::pos(std::size_t toBit)
{
    _written_bits = toBit;
}
