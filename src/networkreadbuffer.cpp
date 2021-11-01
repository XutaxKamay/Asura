#include "networkreadbuffer.h"

XLib::NetworkReadBuffer::NetworkReadBuffer(XLib::data_t data,
                                           XLib::safesize_t maxSize,
                                           XLib::safesize_t readBits)
 : Buffer(data, maxSize), _read_bits(readBits)
{
}

auto XLib::NetworkReadBuffer::readBit() -> bool
{
    if (_read_bits / CHAR_BIT >= maxSize())
    {
        XLIB_EXCEPTION("Filled buffer");
    }

    return read_bit(data(), _read_bits++);
}

void XLib::NetworkReadBuffer::pos(XLib::safesize_t toBit)
{
    _read_bits = toBit;
}
