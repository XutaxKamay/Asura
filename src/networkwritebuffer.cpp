#include "pch.h"

#include "networkwritebuffer.h"

XKLib::NetworkWriteBuffer::NetworkWriteBuffer(XKLib::data_t data,
                                              std::size_t maxSize,
                                              std::size_t writtenBits)
 : Buffer(data, maxSize),
   _written_bits(writtenBits)
{
}

void XKLib::NetworkWriteBuffer::writeBit(bool value)
{
    if (_written_bits / CHAR_BIT >= maxSize())
    {
        XKLIB_EXCEPTION("Filled buffer");
    }

    write_bit(data(), _written_bits++, value);
}

void XKLib::NetworkWriteBuffer::pos(std::size_t toBit)
{
    _written_bits = toBit;
}
