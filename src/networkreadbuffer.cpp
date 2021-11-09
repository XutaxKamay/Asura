#include "pch.h"

#include "networkreadbuffer.h"

XKLib::NetworkReadBuffer::NetworkReadBuffer(XKLib::data_t data,
                                            std::size_t maxSize,
                                            std::size_t readBits)
 : Buffer(data, maxSize), _read_bits(readBits)
{
}

auto XKLib::NetworkReadBuffer::readBit() -> bool
{
    if (_read_bits / CHAR_BIT >= maxSize())
    {
        XKLIB_EXCEPTION("Filled buffer");
    }

    return read_bit(data(), _read_bits++);
}

void XKLib::NetworkReadBuffer::pos(std::size_t toBit)
{
    _read_bits = toBit;
}
