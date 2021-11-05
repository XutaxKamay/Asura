#include "pch.h"

#include "readbuffer.h"

using namespace XKLib;

ReadBuffer::ReadBuffer(data_t data, size_t maxSize, size_t readSize)
 : Buffer(data, maxSize), _read_size(readSize)
{
}

auto ReadBuffer::reset() -> void
{
    _read_size = 0;
}

auto ReadBuffer::advance(size_t size) -> void
{
    _read_size += size;
}

auto ReadBuffer::readSize() -> size_t
{
    return _read_size;
}

auto ReadBuffer::setReadSize(size_t readSize)
{
    _read_size = readSize;
}
