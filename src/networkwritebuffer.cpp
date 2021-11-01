#include "networkwritebuffer.h"

XLib::NetworkWriteBuffer::NetworkWriteBuffer(XLib::data_t data,
                                             size_t maxSize,
                                             size_t writtenBits)
 : Buffer(data, maxSize), _written_bits(writtenBits)
{
}

void XLib::NetworkWriteBuffer::writeBit(bool value)
{
    if (_written_bits / CHAR_BIT >= maxSize())
    {
        XLIB_EXCEPTION("Filled buffer");
    }

    write_bit(data(), _written_bits++, value);
}

void XLib::NetworkWriteBuffer::pos(size_t toBit)
{
    _written_bits = toBit;
}
