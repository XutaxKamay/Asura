#include "networkreadbuffer.h"

XKLib::NetworkReadBuffer::NetworkReadBuffer(XKLib::data_t data,
                                            size_t maxSize,
                                            size_t readBits)
 : Buffer(data, maxSize), _read_bits(readBits)
{
}

auto XKLib::NetworkReadBuffer::readBit() -> bool
{
    if (_read_bits / CHAR_BIT >= maxSize())
    {
        XLIB_EXCEPTION("Filled buffer");
    }

    return read_bit(data(), _read_bits++);
}

void XKLib::NetworkReadBuffer::pos(size_t toBit)
{
    _read_bits = toBit;
}
