#include "networkwritebuffer.h"

XLib::NetworkWriteBuffer::NetworkWriteBuffer(XLib::data_t data,
                                             XLib::safesize_t maxSize,
                                             XLib::safesize_t writtenBits)
 : Buffer(data, maxSize), _written_bits(writtenBits)
{
}

void XLib::NetworkWriteBuffer::writeBit(bool value)
{
    if (_written_bits / 8 >= maxSize())
    {
        XLIB_EXCEPTION("Filled buffer");
    }

    write_bit(data(), _written_bits++, value);
}

void XLib::NetworkWriteBuffer::pos(XLib::safesize_t toBit)
{
    _written_bits = toBit;
}
