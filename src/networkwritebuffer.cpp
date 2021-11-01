#include "networkwritebuffer.h"

XKLib::NetworkWriteBuffer::NetworkWriteBuffer(XKLib::data_t data,
                                              size_t maxSize,
                                              size_t writtenBits)
 : Buffer(data, maxSize), _written_bits(writtenBits)
{
}

void XKLib::NetworkWriteBuffer::writeBit(bool value)
{
    if (_written_bits / CHAR_BIT >= maxSize())
    {
        XLIB_EXCEPTION("Filled buffer");
    }

    write_bit(data(), _written_bits++, value);
}

void XKLib::NetworkWriteBuffer::pos(size_t toBit)
{
    _written_bits = toBit;
}
